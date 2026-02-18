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
    uri: "gh:hicknhack-software/SourceMap-Cpp#ee4759af5e939a2f40420915b26c3abcfca6e797"
  }
  online.QbsProject {
    source: sourceMap

    Properties {
      configProductName: root.parent.configProductName
    }
  }
}
