Project {
  name: "ThirdParty"

  // note: provided through conan for now
  // OnlineProduct {
  //     name: "nlohmann_json"
  //     uri: "gh:nlohmann/json@3.12.0"
  // }
  // note: You need the Qbs patch from https://github.com/hicknhack-software/Qt-Qbs/tree/feature/OnlineSource
  OnlineSubProject {
      name: "SourceMap"
      uri: "gh:hicknhack-software/SourceMap-Cpp#610de35e3fb38d5286e5dea56c7abf21e7637dc7"
  }
}
