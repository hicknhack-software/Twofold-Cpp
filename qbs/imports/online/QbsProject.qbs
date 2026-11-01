// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: MIT

SubProject {
    property variant source

    condition: source && source.found
    filePath: source.projectFilePath
}
