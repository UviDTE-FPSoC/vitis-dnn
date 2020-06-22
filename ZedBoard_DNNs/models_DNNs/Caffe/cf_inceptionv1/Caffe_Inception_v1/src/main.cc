#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <ctime>
#include <queue>
#include <string>
#include <cstring>
#include <vector>

/* header file OpenCV for image processing */
#include <opencv2/opencv.hpp>

/* header file for DNNDK APIs */
#include <dnndk/dnndk.h>

using namespace std;
using namespace cv;

/* Name of the DPU kernel generated by the DNNC compiler */
#define KERNEL_DNN "inception_v1_0"
/* Input Node for Kernel Inception_v1 */
#define INPUT_NODE      "conv1_7x7_s2"
/* Output Node for Kernel Inception_v1 */
#define OUTPUT_NODE     "loss3_classifier"
/* File where the execution time results are going to be writed to */
#define OUTPUT_FILENAME "cf_inceptionv1_ImageInput.txt"

/* Path to the images in the target board considering this application is stored in a folder within the ZedBoard directory. */
const string inferenceImages = "../../test_images/";

/* Vectors to store ImageNet test images solutions */
vector<int> solutions;



/**
 * @brief load the 1000 ImageNet image names and the solutions for each of the validation images into two separate vectors
 *
 * @param path - path to the file with the images and solution names
 * @param image - the vector of iamage names
 * @param solutions - the vector of solutions int
 *
 * @return none
 */
void LoadImageNamesSolutions(string const &path, vector<string> &images, vector<int> &solutions) {
    images.clear();
    solutions.clear();


    fstream fimgsol(path + "val.txt");	// A variable fimgsol of the fstream datatype is constructed with the path to the .txt file with the image names.
    if (fimgsol.fail()) {
        fprintf(stderr, "Error : Open %s failed.\n", path.c_str());
        exit(1);
    }
    string label;
    while (getline(fimgsol, label)) {	// getline() reads a line from the file opened with fstream and writes it to the label variable.
        
        /* Copy a line of the fimgsol file to a char variable */
        char * writable = new char[label.size() + 1];
        std::copy(label.begin(), label.end(), writable);

        /* Add a 0 at the end of the char variable to have a string in C format */
        writable[label.size()] = '\0';

        /* With a string in C format you can use the strtok() function to separate each word in the char variable separated by a space, comma, dot or dash */
        char * pch = strtok(writable, " \n");
        int pch_count = 0;
        while (pch != NULL)
        {
            if (pch_count == 0) {
                images.push_back(pch);
                pch_count = 1;
            } else {
                solutions.push_back(atoi(pch));
            }
            pch = strtok (NULL, " ,.-");
        }

        delete[] writable;
    }

    /*for (int j = 0; j < 20; j = j + 1) {
        cout << "Image: " << images[j] << " --- Solution: " << solutions[j] << endl;
    }*/

    fimgsol.close();	// Close the file when it has been read.
}



/**
 * @brief load labels from file to a vector
 *
 * @param path - path to the labels file
 * @param labels - the vector of labels string
 *
 * @return none
 */
void LoadWords(string const &path, vector<string> &labels) {
    labels.clear();


    fstream flabels(path);	// A variable flabels of the fstream datatype is constructed with the path to the .txt file with the labes.
    if (flabels.fail()) {
        fprintf(stderr, "Error : Open %s failed.\n", path.c_str());
        exit(1);
    }
    string label;
    while (getline(flabels, label)) {	// getline() reads a line from the file opened with fstream and writes it to the label variable.
        labels.push_back(label);
    }

    flabels.close();	// Close the file when it has been read.
}


/**
 * @brief resize the shortes size of the image to 256 and the other size with the same proportion, in order to keep the aspect ratio
 *
 * @param image - reference to a Mat object that contains the image
 * 
 * @return none
 */
void short_size_resize(Mat &image) {

    /* Retrieve actual image size */
    int height = image.size().height;
    int width = image.size().width;
    
    float scale = 0;
    int proportional_side = 0;

    if (height > width) {   // If the width is the shortest side, resize it to 256
        scale = 256.0/width;
        proportional_side = height*scale;
        resize(image, image, Size(256, proportional_side));
    } else if (height < width) {   // If the height is the shortest side, resize it to 256
        scale = 256.0/height;
        proportional_side = width*scale;
        resize(image, image, Size(proportional_side, 256));
    } else {    // If they are equal, resize both
        resize(image, image, Size(256, 256));
    }
}


/**
 * @brief perform a central crop to the input size
 *
 * @param image - reference to a Mat object that contains the image
 * @param crop_width - output image width
 * @param crop_height - output image height
 * 
 * @return none
 */
void central_crop(Mat &image, int &crop_width, int &crop_height) {

    /* Obtain the center points of the image */
    int centre_y = image.size().height/2;
    int centre_x = image.size().width/2;

    /* Establish the four corners of the cropped down image */
    int left_x = centre_x - crop_width/2;
    int right_x = centre_x + crop_width/2;
    int top_y = centre_y - crop_height/2;
    int bottom_y = centre_y + crop_height/2;

    /* Create a rectangle with the four corners previously calculated */
    Rect cropSurface(left_x, top_y, right_x, bottom_y);	// The two first attributes are the x and y coordinates of the top left corner of the rectangle

    /* Crop the image with the rectanble */
    image = image(cropSurface);
}


/**
 * @brief perform the mean value substraction to each BGR color layer
 *
 * @param image - reference to a Mat object that contains the image
 * 
 * @return none
 */
void mean_value_substract(Mat &image) {
    
    subtract(image, Scalar(104, 117, 123), image);

}



/**
 * @brief perform image pre-processing for TensorFlow Inceptionv1
 *
 * @param image - reference to the mat object containing an image in RGB format
 * @param taskDNN - pointer to the neural network task Task
 * @param inputTensor_width - reference to the input tensor width
 * @param inputTensor_height - reference to the input tensor height
 *
 * @return none
 */
void preprocessing(Mat &image, int &inputTensor_width, int &inputTensor_height) {
    
    /* Resize the shortest size to 256, keeping the aspect ratio, which means resizing the other size proportionally. */
    short_size_resize(image);

    /* Centre crop to the actual size of the input tensor, 224x224 for Inceptionv1 */
    central_crop(image, inputTensor_width, inputTensor_height);

    /* Substract the mean value of each color layer */
    mean_value_substract(image);
}




/**
 * @brief calculate the softmax layer
 *
 * @param data - pointer to input buffer
 * @param size - size of input buffer
 * @param result - pointer to a result array with the size of the ouput tensor channels
 *
 * @return none
 */
void CPUCalcSoftmax(const float *data, size_t size, float *result) {
    assert(data && result);
    double sum = 0.0f;

    /* Implementation of softmax function */
    for (size_t i = 0; i < size; i++) {
        result[i] = exp(data[i]);
        sum += result[i];
    }

    for (size_t i = 0; i < size; i++) {
        result[i] /= sum;
    }
}



/**
 * @brief Get top k results according to its probability
 *
 * @param d - pointer to input data
 * @param size - size of input data
 * @param k - calculation result
 * @param vkinds - vector of kinds
 * @param sol_counter - counter to calculate the correct solution
 * @param top1 - counter to calculate top1 metrics
 * @param top5 - counter to calculate top5 metrics
 *
 * @return none
 */
void TopK(const float *d, int size, int k, vector<string> &vkinds, int &sol_counter, int &top1, int &top5) {
    assert(d && size > 0 && k > 0);
    priority_queue<pair<float, int>> q;

    for (auto i = 0; i < size; ++i) {
        q.push(pair<float, int>(d[i], i));
    }

    for (auto i = 0; i < k; ++i) {
        pair<float, int> ki = q.top();
        printf("top[%d] prob = %-8f  name = %s\n", i, d[ki.second],
        vkinds[ki.second].c_str());

        /* Check if the correct solution is in the top 1 answers */

        /* Check if the correct solution is in the top 5 answers */
        if (ki.second == solutions[sol_counter]) {
            top5 = top5 + 1;
            if (i == 0) {
                top1 = top1 + 1;
            }
        }
        q.pop();
    }
    cout << "Top1 count = " << top1 << " -------- Top5 count = " << top5 << endl;
}



/**
 * @brief Run DPU Task for the selected neural network
 *
 * @param taskDNN - pointer to the neural network task Task
 *
 * @return none
 */
void runDNN(DPUTask *taskDNN) {

    /* In the task pointer doesn't contain a pointer, the function is aborted. */
    assert(taskDNN);

    /* Create a vector of strings to save the image names (images) and the ImageNet labels.
       The vector and string datatype belong to the std name space (std::vector<std::string>). */
    vector<string> labels, images;

    /* Load all image names and image solutions. If no names or solutions are loaded, exit the void function.*/
    LoadImageNamesSolutions(inferenceImages, images, solutions);
    if (images.size() == 0) {
        cerr << "\nError: No images existing under " << inferenceImages << endl;
        return;
    }
    if (solutions.size() == 0) {
        cerr << "\nError: No solutions existing under " << inferenceImages << endl;
        return;
    }

    /* Load all labels words. This words are contained in a file and correspond to all the possible labels of the images. */
    LoadWords(inferenceImages + "words.txt", labels);
    if (labels.size() == 0) {
        cerr << "\nError: No words exist in file words.txt." << endl;
        return;
    }

    /* Get size of the input tensor */
    int inputTensor_height = dpuGetInputTensorHeight(taskDNN, INPUT_NODE);
    int inputTensor_width = dpuGetInputTensorWidth(taskDNN, INPUT_NODE);

    /* Get channel count of the output Tensor for the DNN Task  */
    int channel = dpuGetOutputTensorChannel(taskDNN, OUTPUT_NODE);
    float *FCResult = new float[channel];
    float *softmax = new float[channel];

    /* Create file to store execution inference results */
    ofstream fs(OUTPUT_FILENAME);
    fs.close();

    /* Counter to print correct solution for the 500 images */
    int sol_counter = 0;

    /* Counters to calculate top1 and top5 metrics */
    int top1 = 0;
    int top5 = 0;


    for (auto &imageName : images) {	// Expresion to go through out all the iterations of the images vector.
        cout << "\nLoad image : " << imageName << endl;

        /* Load image and Set image into DPU Task for the DNN */
        Mat image = imread(inferenceImages + imageName);       
        
        /* Perform image preprocessing */
        preprocessing(image, inputTensor_width, inputTensor_height);

        /* Initialize and start timer */
        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        std::chrono::duration<double> elapsed_seconds;
        start = std::chrono::high_resolution_clock::now();

        /* Set image data into DPU task */
        dpuSetInputImage2(taskDNN, INPUT_NODE, image);

        /* Launch the DNN Task */
        dpuRunTask(taskDNN);

        /* Get DPU execution time (in us) of DPU Task */
        //long long timeProf = dpuGetTaskProfile(taskDNN);
        //cout << "  DPU Task Execution time: " << (timeProf * 1.0f) << "us\n";

        /* Get FC (Fully Connected layer) result and convert from INT8 to FP32 format. This is equal to spatial squeeze */
        dpuGetOutputTensorInHWCFP32(taskDNN, OUTPUT_NODE, FCResult, channel);

        /* Calculate softmax on CPU */
        CPUCalcSoftmax(FCResult, channel, softmax);

        /* Stop the timer and calculate the execution time */
        end = std::chrono::high_resolution_clock::now();
        elapsed_seconds = end - start;

        /* Write execution time results in miliseconds to file */
        fs.open(OUTPUT_FILENAME, std::fstream::out | std::fstream::app);
        if(!fs)
        {
            cerr<<"Cannot open the output file."<<std::endl;
        } else{
            fs << elapsed_seconds.count()*1000 << "\n";
            fs.close();
            cout << "\nInput writen to textfile" << endl;
        }

 	    /* Display TOP-5 classification results and the correct solution */
        TopK(softmax, channel, 5, labels, sol_counter, top1, top5);
        cout << "The correct solution is " << labels[solutions[sol_counter]] << endl;
        sol_counter = sol_counter + 1;
        cout << "The top1 accuracy is " << float(top1)/sol_counter << " --------- The top5 accuracy is " << float(top5)/sol_counter << endl;
    }

    float top1_metric = float(top1)/sol_counter;
    float top5_metric = float(top5)/sol_counter;

    cout << "The top1 accuracy is " << top1_metric << endl;
    cout << "The top5 accuracy is " << top5_metric << endl;
    delete[] softmax;
    delete[] FCResult;
}




/**
 * @brief Entry for runing a deep neural network
 *
 * @note Functions prefixed with "dpu" are imported as DNNDK APIs to reduce the
 *
 *       complexity of programing a neural network application for the DPU.
 *
 */
int main(void) {
    /* DPU Kernel/Task for running the DNN. A pointer is created for both the kernel and the task with the corresponding datatype. */
    DPUKernel *kernelDNN;
    DPUTask *taskDNN;

    /* Attach and open DPU device file “/dev/dpu” before the utilization of DPU resources.
       Returns a 0 if succesfull or a negative number in case of error. */
    dpuOpen();

    /* Load DPU Kernel for the network. KERNEL_DNN contains a string with the 
       name of the kernel outputed by the DNNC compiler */
    kernelDNN = dpuLoadKernel(KERNEL_DNN);

    /* Create DPU Task for the neural network. The arguments indicated are a pointer to the 
       DPU kernel and the indication for Normal mode. */
    taskDNN = dpuCreateTask(kernelDNN, 0);

    /* Run the deep neural network Task */
    runDNN(taskDNN);

    /* Destroy DPU Task & free resources */
    dpuDestroyTask(taskDNN);

    /* Destroy DPU Kernel & free resources */
    dpuDestroyKernel(kernelDNN);

    /* Dettach from DPU driver & free resources */
    dpuClose();

    return 0;
}