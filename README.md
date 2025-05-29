# tgd - multidimensional arrays in C++

Tagged Grid Data (TGD) is a library to make working with multidimensional
arrays in C++ easy.

For example:
```c++
TGD::Array<uint8_t> image({ 640, 480 }, 3);
image[{ x, y }][0] = redValue;
image[{ x, y }][1] = greenValue;
image[{ x, y }][2] = blueValue;
TGD::save(image, "image.png");
```

The core TGD library is header-only; you just include `<tgd/array.hpp>`.

For input and output you can optionally link against libtgd. This library has
builtin support for various file formats including png, jpeg, exr, hdr, pfm,
ppm, bmp, tga, csv, and the native tgd format.

No external libraries are required. Optionally, additional libraries can be
used to support file formats such as tiff, dicom, hdf5, pdf, pfs, fits, mp4 and
others.

There is also a command line utility named `tgd` that can create, convert and
modify files in supported formats, and print information about them. The `tgd`
utility does not require any libraries either, but can use the muparser library
for its `calc` command.

This project uses the MIT license.
