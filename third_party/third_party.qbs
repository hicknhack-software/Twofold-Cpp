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
    uri: "gh:hicknhack-software/SourceMap-Cpp#46554679459e0a79de3fd378a700f24b2287b292"
  }
  online.QbsProject {
    source: sourceMap

    Properties {
      configProductName: root.parent.configProductName
    }
  }
}
