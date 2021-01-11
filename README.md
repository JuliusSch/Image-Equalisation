# Image Equalisation Program

Both local and global parallel implementations of equalisation algorithms to correct image contrast, using the OpenCL framework. Supports variable bin size and number.
The algorithm works by sorting each colour that appears in the image into a bin, then once each colour total has been found this data set is normalised and the image reconstructed. Both colour and greyscale images can be operated on (*.ppm* and *.pgm* files respectively).

## Demo

Before                     |  After
:-------------------------:|:-------------------------:
![input1](demo/test_1.png) |  ![output1](demo/output1.PNG)
In colour           |  
![input2](demo/test_2.png) |  ![output2](demo/output2.PNG)
With a reduced bin number (i.e. fewer colours in the output)  |
![input3](demo/test_2.png) |  ![output3](demo/output3.PNG)
