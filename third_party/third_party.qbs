import online

Project {
  id: root

  name: "ThirdParty"

  // note: potential override of source versions
  // online.Source {
  //   id: source
  //   name: "catch2"
  //   uri: "gh:catchorg/catch2@3.12.0"
  // }
  // online.Source {
  //   id: source
  //   name: "nlohmann_json"
  //   uri: "gh:nlohmann/json@3.12.0"
  // }

  online.Source {
    id: sourceMap

    name: "SourceMap"
    uri: "gh:hicknhack-software/SourceMap-Cpp#f3b56aa61195cfbe5066bc49e9bf19e5e695caa1"
  }
  online.QbsProject {
    source: sourceMap

    Properties {
      configProductName: root.parent.configProductName
    }
  }
}
