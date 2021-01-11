# Image contrast adjustment using histogram equalisation

Both local and global parallel implementations of histogram equalisation algorithms to correct image contrast, using the OpenCL framework. Supports variable bin size and number. the parallel implementation increases the speed of the operations dramatically, especially for large images. The algorithm works by generating an image histogram, then normalising the distribution of colours to spread out the most frequent values. Both colour and greyscale images can be operated on (*.ppm* and *.pgm* files respectively).

For more on histogram equalisation see [here](https://en.wikipedia.org/wiki/Histogram_equalization).

## Demo

Before                     |  After
:-------------------------:|:-------------------------:
![input1](demo/test_1.png) |  ![output1](demo/output1.PNG)
In colour           |  
![input2](demo/test_2.png) |  ![output2](demo/output2.PNG)
With a reduced bin number (i.e. fewer colours in the output)  |
![input3](demo/test_2.png) |  ![output3](demo/output3.PNG)
