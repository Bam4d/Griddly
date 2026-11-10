#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <vector>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/VectorizedEnv.hpp"
#include "../../src/Griddly/Core/Observers/TiledRenderer.hpp"

namespace py = nanobind;

namespace griddly {

// Python wrapper around VectorizedEnv (+ optional TiledRenderer, SDL or CUDA backend).
//
// step() takes an ndarray of shape (num_envs, num_actions, action_len) with
// the external action input format, releases the GIL while the C++ thread
// pool runs, and returns (rewards, terminated, wins) as ndarrays.
//
// observe() renders all envs into one tiled frame and returns a torch tensor
// via DLPack: a host tensor, or with device=True a CUDA tensor that is never
// copied off the GPU (CUDA backend, or the SDL GPU renderer on a CUDA GPU).
class Py_VectorizedWrapper {
 public:
  Py_VectorizedWrapper(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs, uint32_t numThreads,
                       std::string observerName, std::string renderBackend = "auto")
      : gdyFactory_(gdyFactory), observerName_(observerName), renderBackend_(parseBackend(renderBackend)) {
    env_ = std::make_unique<VectorizedEnv>(gdyFactory, numEnvs, numThreads);
    {
      py::gil_scoped_release release;
      env_->init();
    }
  }

  uint32_t numEnvs() const { return env_->numEnvs(); }
  uint32_t numThreads() const { return env_->numThreads(); }

  void loadLevel(uint32_t levelId) {
    py::gil_scoped_release release;
    env_->loadLevel(levelId);
  }

  void loadLevelString(const std::string& levelString) {
    py::gil_scoped_release release;
    env_->loadLevelString(levelString);
  }

  void reset() {
    py::gil_scoped_release release;
    env_->reset();
  }

  void resetFromSnapshotAll() {
    py::gil_scoped_release release;
    env_->resetFromSnapshotAll();
  }

  void seed(uint32_t seed) {
    py::gil_scoped_release release;
    env_->seed(seed);
  }

  // actions: (num_envs, max_actions, action_len) int32 ndarray (padded rows ok;
  // action_len 0 rows are ignored). Returns (rewards, terminated, wins).
  py::tuple step(py::ndarray<int32_t, py::ndim<3>> actions) {
    if (actions.ndim() != 3) throw std::invalid_argument("actions must be (num_envs, max_actions, action_len)");
    const uint32_t n = env_->numEnvs();
    if (actions.shape(0) != n) throw std::invalid_argument("actions.shape(0) must equal num_envs");
    const uint32_t maxActs = (uint32_t)actions.shape(1);
    const uint32_t actLen = (uint32_t)actions.shape(2);

    std::vector<std::vector<std::vector<int32_t>>> perEnv(n);
    for (uint32_t e = 0; e < n; e++) {
      for (uint32_t a = 0; a < maxActs; a++) {
        std::vector<int32_t> row;
        row.reserve(actLen);
        bool any = false;
        for (uint32_t k = 0; k < actLen; k++) {
          int32_t v = actions(e, a, k);
          if (v != 0) any = true;
          row.push_back(v);
        }
        if (any) perEnv[e].push_back(std::move(row));
      }
    }

    VectorizedStepResult res;
    {
      py::gil_scoped_release release;  // thread pool runs free of the GIL
      res = env_->step(perEnv);
    }

    // Own the memory in heap vectors; a capsule with a deleter keeps them
    // alive as long as any returned ndarray holds the owner handle.
    auto* rv = new std::vector<int32_t>(n);
    auto* tv = new std::vector<uint8_t>(n);
    auto* wv = new std::vector<uint8_t>(n);
    for (uint32_t i = 0; i < n; i++) {
      (*rv)[i] = res.rewards[i];
      (*tv)[i] = res.terminated[i] ? 1 : 0;
      (*wv)[i] = res.wins[i] ? 1 : 0;
    }
    auto* holder = new std::tuple<std::vector<int32_t>*, std::vector<uint8_t>*, std::vector<uint8_t>*>{rv, tv, wv};
    py::capsule owner(holder, [](void* p) noexcept {
      auto* h = (std::tuple<std::vector<int32_t>*, std::vector<uint8_t>*, std::vector<uint8_t>*>*)p;
      delete std::get<0>(*h);
      delete std::get<1>(*h);
      delete std::get<2>(*h);
      delete h;
    });
    size_t shape1[1] = {n};
    auto rewards = py::ndarray<int32_t, py::ndim<1>, py::numpy>(rv->data(), 1, shape1, owner);
    auto terminated = py::ndarray<uint8_t, py::ndim<1>, py::numpy>(tv->data(), 1, shape1, owner);
    auto wins = py::ndarray<uint8_t, py::ndim<1>, py::numpy>(wv->data(), 1, shape1, owner);
    return py::make_tuple(py::cast(rewards), py::cast(terminated), py::cast(wins));
  }

  // Render all envs into one tiled frame; return as a torch tensor via DLPack.
  // Shape: (rows*envH + gaps, cols*envW + gaps, 4) uint8 RGBA.
  // device=True returns the CUDA-resident frame (valid until the next observe);
  // raises if the backend has no CUDA output (SDL on CPU or a non-CUDA GPU).
  py::object observe(bool parallel = true, bool device = false) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!renderer_) {
      std::vector<std::shared_ptr<Grid>> grids;
      grids.reserve(env_->numEnvs());
      for (uint32_t i = 0; i < env_->numEnvs(); i++) grids.push_back(env_->grid(i));
      TiledRendererConfig cfg;
      cfg.observerName = observerName_;
      cfg.backend = renderBackend_;
      renderer_ = createTiledRenderer(gdyFactory_, env_->numEnvs(), grids, cfg);
    }
    const uint8_t* buf;
    {
      py::gil_scoped_release release;
      buf = device ? renderer_->renderDevice(parallel, 0) : renderer_->render(parallel, 0);
    }
    const uint32_t w = renderer_->width();
    const uint32_t h = renderer_->height();

    // DLPack capsule: torch.from_dlpack() / torch.utils.dlpack.from_dlpack()
    // build a torch tensor that shares this memory.
    size_t shape[3] = {h, w, 4};
    size_t strides[3] = {w * 4, 4, 1};
    constexpr int kDLUInt = 1;   // DLDataTypeCode::kDLUInt (kDLInt is 0)
    constexpr int kDLCPU = 1;    // DLDeviceType::kDLCPU
    constexpr int kDLCUDA = 2;   // DLDeviceType::kDLCUDA
    if (device) return makeDlpackTensor(buf, shape, strides, 3, kDLUInt, kDLCUDA, renderer_->deviceId());
    return makeDlpackTensor(buf, shape, strides, 3, kDLUInt, kDLCPU, 0);
  }

  // "sdl" or "cuda" once observe() has created the renderer, else ""
  std::string renderBackend() const {
    if (!renderer_) return "";
    return renderer_->backend() == TiledRenderBackend::CUDA ? "cuda" : "sdl";
  }

  uint32_t frameWidth() const { return renderer_ ? renderer_->width() : 0; }
  uint32_t frameHeight() const { return renderer_ ? renderer_->height() : 0; }

  // nanobind needs a movable type for the shared_ptr holder; the mutex makes
  // the implicit move deleted, so define it manually.
  Py_VectorizedWrapper(Py_VectorizedWrapper&& o) noexcept
      : gdyFactory_(std::move(o.gdyFactory_)),
        observerName_(std::move(o.observerName_)),
        renderBackend_(o.renderBackend_),
        env_(std::move(o.env_)),
        renderer_(std::move(o.renderer_)) {}
  Py_VectorizedWrapper& operator=(Py_VectorizedWrapper&&) = delete;

 private:
  static TiledRenderBackend parseBackend(const std::string& name) {
    if (name == "auto") return TiledRenderBackend::AUTO;
    if (name == "sdl") return TiledRenderBackend::SDL;
    if (name == "cuda") return TiledRenderBackend::CUDA;
    throw std::invalid_argument("render_backend must be 'auto', 'sdl' or 'cuda', got '" + name + "'");
  }

  // Build a DLPack capsule wrapping `ptr` with the given shape/strides.
  // Uses the standard DLPack C ABI so torch.from_dlpack() accepts it.
  static py::object makeDlpackTensor(const void* ptr, size_t* shape, size_t* strides,
                                     int ndim, int dtypeCode, int deviceType, int deviceId) {
    // DLpack structures (packed, C ABI)
    struct DLDataType { uint8_t code; uint8_t bits; uint16_t lanes; };
    struct DLDevice { int device_type; int device_id; };
    struct DLTensor {
      void* data;
      DLDevice device;
      int ndim;
      DLDataType dtype;
      int64_t* shape;
      int64_t* strides;
      uint64_t byte_offset;
    };
    struct DLManagedTensor {
      DLTensor dl_tensor;
      void* manager_ctx;
      void (*deleter)(struct DLManagedTensor* self);
    };

    auto* mt = new DLManagedTensor{};
    mt->dl_tensor.data = const_cast<void*>(ptr);
    mt->dl_tensor.device = {(int)deviceType, (int)deviceId};
    mt->dl_tensor.ndim = ndim;
    mt->dl_tensor.dtype = {(uint8_t)dtypeCode, 8, 1};  // kDLUInt=1 -> uint8
    mt->dl_tensor.shape = new int64_t[ndim];
    mt->dl_tensor.strides = new int64_t[ndim];
    for (int i = 0; i < ndim; i++) {
      mt->dl_tensor.shape[i] = (int64_t)shape[i];
      mt->dl_tensor.strides[i] = (int64_t)strides[i];
    }
    mt->dl_tensor.byte_offset = 0;
    mt->manager_ctx = nullptr;
    mt->deleter = [](DLManagedTensor* self) {
      delete[] self->dl_tensor.shape;
      delete[] self->dl_tensor.strides;
      delete self;
    };

    // Capsule name must be "dltensor" for torch.from_dlpack
    return py::capsule(mt, "dltensor");
  }

  std::shared_ptr<GDYFactory> gdyFactory_;
  std::string observerName_;
  TiledRenderBackend renderBackend_ = TiledRenderBackend::AUTO;
  std::unique_ptr<VectorizedEnv> env_;
  std::unique_ptr<TiledRenderer> renderer_;
 private:
  std::mutex mutex_;
};

}  // namespace griddly
