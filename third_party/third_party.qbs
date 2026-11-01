// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
    uri: "gh:hicknhack-software/SourceMap-Cpp#a6ec7c1cec1d3e359f795eea064a686ae8f8a6c8"
  }
  online.QbsProject {
    source: sourceMap

    Properties {
      configProductName: root.parent.configProductName
    }
  }
}
