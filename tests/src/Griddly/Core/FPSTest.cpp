// #include <chrono>

// #include "Griddly/Core/GDY/Actions/Action.cpp"
// #include "Griddly/Core/GDY/Objects/ObjectGenerator.cpp"
// #include "Griddly/Core/Grid.cpp"
// #include "Griddly/Core/Players/Player.cpp"
// #include "Griddly/Core/TurnBasedGameProcess.cpp"
// #include "gtest/gtest.h"
// #include "spdlog/spdlog.h"

// namespace griddly {

// TEST(FPSTest, SimpleFPSTest) {
//   auto grid = std::shared_ptr<Grid>(new Grid());
//   auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
//   auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
//   auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator, {"", ""}));

//   gdyFactory->initializeFromFile("resources/games/RTS/GriddlyRTS.yaml");
//   auto observerName = "VECTOR";
//   auto globalObserverName = "VECTOR";

//   auto gameProcess = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(globalObserverName, gdyFactory, grid));

//   auto player1 = std::shared_ptr<Player>(new Player(1, "TestPlayer", observerName, gameProcess));
//   auto player2 = std::shared_ptr<Player>(new Player(2, "TestPlayer2", observerName, gameProcess));

//   gameProcess->addPlayer(player1);
//   gameProcess->addPlayer(player2);

//   gameProcess->init();

//   for (int x = 0; x < 10; x++) {
//     uint64_t frames = 0;
//     uint64_t durationMs;
//     auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//     while (true) {
//       auto action1 = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), "move", 0));
//       action1->init(glm::vec2{0, 1}, glm::vec2{0, 1});

//       gameProcess->performActions(1, {action1});
//       auto action2 = std::shared_ptr<Action>(new Action(gameProcess->getGrid(), "move", 0));
//       action2->init(glm::vec2{0, 1}, glm::vec2{0, 1});
//       gameProcess->performActions(1, {action2});

//       if (frames > 0 && frames % 1000 == 0) {
//         auto end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//         durationMs = end - start;
//         if (durationMs > 1000) {
//           spdlog::info("fps: {}", (double)frames / ((double)durationMs/1000));
//           break;
//         }
//       }

//       frames++;
//     }
//   }
// }

// }  // namespace griddly