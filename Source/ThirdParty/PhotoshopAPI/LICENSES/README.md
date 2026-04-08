# PhotoshopAPI ThirdParty Licenses

This directory MUST contain the license file for PhotoshopAPI and each transitive
dependency whose static library is vendored under `../Win64/lib/`. License files
are committed alongside the binaries per the vendoring policy described in
`.planning/phases/02-c-psd-parser/02-CONTEXT.md` (PhotoshopAPI Vendoring).

Expected license files (drop the real upstream LICENSE/COPYING files here during
the one-time bootstrap run of `Scripts/build-photoshopapi.bat`):

| Library        | License   | Filename (suggested)      |
| -------------- | --------- | ------------------------- |
| PhotoshopAPI   | MIT       | `PhotoshopAPI-LICENSE`    |
| OpenImageIO    | BSD-3     | `OpenImageIO-LICENSE`     |
| fmt            | MIT       | `fmt-LICENSE`             |
| libdeflate     | MIT       | `libdeflate-LICENSE`      |
| stduuid        | MIT       | `stduuid-LICENSE`         |
| Eigen          | MPL-2.0   | `Eigen-LICENSE`           |
| blosc2         | BSD-3     | `blosc2-LICENSE`          |
| zlib-ng        | zlib      | `zlib-ng-LICENSE`         |
| simdutf        | Apache-2  | `simdutf-LICENSE`         |

If additional transitive libraries are pulled in by vcpkg at bootstrap time,
add their license files here as well. The PSD2UMG plugin ships under MIT and
must carry attribution for every statically-linked dependency.
