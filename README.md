# Hilbert-Image-to-Sound
Turn an image into a sequence of sounds using an Hilbert Curve.

Works with black and white, 256x256 square images.
Uses notes from B7 (3951Hz) to A2 (110Hz), a total of 52 tones.

You can convert any image into a supported one through the following ImageMagick command:
convert input.jpg -resize 256x256! -colorspace Gray output.jpg

To use it, enter file name on line 34 of hilbert.cpp, compile it and run it. It will write an output.wav file in the same folder.
