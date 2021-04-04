// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "In3",
    platforms: [
        .macOS(.v10_15),
    ],
    products: [
        // Products define the executables and libraries a package produces, and make them visible to other packages.
        .library(
            name: "In3",
            targets: ["In3"]),
    ],
    targets: [
        // Targets are the basic building blocks of a package. A target can define a module or a test suite.
        // Targets can depend on other targets in this package, and on products in packages this package depends on.
        .systemLibrary(
            name: "CIn3"),
        .target(
            name: "In3",
            dependencies: ["CIn3"],
            linkerSettings: [
                        .unsafeFlags(["-L../build/lib"])
                    ]),
        .testTarget(
            name: "In3Tests",
            dependencies: ["In3"]),
    ]
)
