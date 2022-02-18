
// class EntityObserver : public Observer, public ObservationInterface<u_int8_t> {
//  public:
//   VectorObserver(std::shared_ptr<Grid> grid);
//   ~VectorObserver() override;

//   void init(ObserverConfig observerConfig) override;

//   uint8_t& update() override;
//   void reset() override;
//   void resetShape() override;

//   ObserverType getObserverType() const override;
//   glm::ivec2 getTileSize() const override;

//   void print(std::shared_ptr<uint8_t> observation) override;