#!/usr/bin/env python3
"""Chart CPU vs GPU (EGL/H100) tiled render throughput from the saved sweeps."""
import re, math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

envs = [1,4,8,16,32,64,128,256]
thrs = [1,4,8,16,32]
def parse(f): return [float(x) for x in re.findall(r'(\d+\.\d)\b', open(f).read())]
cpu = parse('benchmarks/h100_render_cpu_2026-11-06.txt')
gpu = parse('benchmarks/h100_render_gpu_egl_2026-11-06.txt')

def frame_px(n):
    cols = math.ceil(math.sqrt(n)); rows = math.ceil(n/cols)
    return (cols*456 + (cols+1)*2) * (rows*432 + (rows+1)*2)

cpu_fps = [max(cpu[i*5:(i+1)*5]) for i in range(8)]
gpu_fps = [max(gpu[i*5:(i+1)*5]) for i in range(8)]
cpu_efr = [f*n for f, n in zip(cpu_fps, envs)]
gpu_efr = [f*n for f, n in zip(gpu_fps, envs)]
cpu_mp  = [f*frame_px(n)/1e6 for f, n in zip(cpu_fps, envs)]
gpu_mp  = [f*frame_px(n)/1e6 for f, n in zip(gpu_fps, envs)]

fig, axes = plt.subplots(1, 3, figsize=(18, 5.5))
x = range(len(envs))
labels = [str(e) for e in envs]

ax = axes[0]
ax.bar([i-0.2 for i in x], cpu_fps, width=0.4, label='CPU software', color='#4C72B0')
ax.bar([i+0.2 for i in x], gpu_fps, width=0.4, label='GPU EGL (H100)', color='#DD8452')
ax.set_yscale('log'); ax.set_xticks(list(x)); ax.set_xticklabels(labels)
ax.set_xlabel('num envs'); ax.set_ylabel('composite frames/s (log)')
ax.set_title('Composite FPS (one frame = all envs tiled)')
ax.legend(); ax.grid(True, which='both', alpha=0.3)
for i,(c,g) in enumerate(zip(cpu_fps, gpu_fps)):
    ax.text(i+0.2, g*1.15, f'{g:.0f}', ha='center', fontsize=8, color='#DD8452')

ax = axes[1]
ax.bar([i-0.2 for i in x], cpu_efr, width=0.4, label='CPU software', color='#4C72B0')
ax.bar([i+0.2 for i in x], gpu_efr, width=0.4, label='GPU EGL (H100)', color='#DD8452')
ax.set_xticks(list(x)); ax.set_xticklabels(labels)
ax.set_xlabel('num envs'); ax.set_ylabel('env-frames/s')
ax.set_title('Per-env throughput (composite fps × envs)')
ax.legend(); ax.grid(True, alpha=0.3)
for i,(c,g) in enumerate(zip(cpu_efr, gpu_efr)):
    ax.text(i+0.2, g*1.02, f'{g/1000:.1f}k', ha='center', fontsize=8, color='#DD8452')

ax = axes[2]
ax.plot(envs, cpu_mp, 'o-', label='CPU software', color='#4C72B0')
ax.plot(envs, gpu_mp, 's-', label='GPU EGL (H100)', color='#DD8452')
ax.set_xscale('log', base=2); ax.set_xticks(envs); ax.set_xticklabels(labels)
ax.set_xlabel('num envs (log2)'); ax.set_ylabel('Mpix/s')
ax.set_title('Raw pixel throughput')
ax.legend(); ax.grid(True, which='both', alpha=0.3)

fig.suptitle('Griddly tiled render on H100 — spiders lvl4 (19×18 tiles @24px), SDL3 opengles2 vs CPU software', y=1.02, fontsize=12)
fig.tight_layout()
out = 'benchmarks/h100_render_cpu_vs_gpu_2026-11-06.png'
fig.savefig(out, dpi=130, bbox_inches='tight')
print('saved', out)
