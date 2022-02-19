

#pragma once

namespace griddly {

template <class ObserverConfigType>
class ObserverConfigInterface {
 public:
  virtual ~ObserverConfigInterface() = default;

  virtual void init(ObserverConfigType& observerConfig) = 0;

  virtual const ObserverConfigType& getConfig() const = 0;
};

}  // namespace griddly