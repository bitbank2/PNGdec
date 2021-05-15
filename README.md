PNGdec
------

What is it?
------------
An 'embedded-friendly' (aka Arduino) PNG image decoding library<br>
<br>

Why did you write it?<br>
-----------------------
Starting in the late 80's, I was fascinated with computer graphics and code optimization. I wrote my own imaging codecs and not long after the PNG specification was ratified, I wrote my own version. I have used it in many projects and products over the years and recently decided that it could get a new lease on life as an embedded-friendly library for microcontrollers.<br>
<br>
What's special about it?<br>
------------------------
The PNG image specification was written at a time when computers had a lot of memory and conserving memory wasn't a priority. The memory allocated for decoding the compressed data (zlib) and for holding the uncompressed image can be quite a bit more than is available on modern microcontrollers. Three goals for this code are: easy to compile+use, use a minimal amount of RAM and be self-contained. One of the dependencies I like to remove when working on embedded software is malloc/free. When compiling on a system with tiny amount of RAM, heap memory management might not even exist.<br>
<br>
Feature summary:<br>
----------------<br>
- Runs on any MCU with at least 48K of free RAM<br>
- No external dependencies (including malloc/free)<br>
- Decode an image line by line with a callback function<br>
- Decode an image to a user supplied buffer (no callback needed)<br>
- Supports all standard options<br>
- Function provided to turn any pixel format into RGB565 for LCD displays<br>
- Arduino-style C++ library class<br>
- Can by built as straight C as well<br>
<br>
