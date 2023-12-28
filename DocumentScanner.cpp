#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std; // To avoid std::string or cv2::imread

//// FOR EVERY PROJECT: Have a CMakeLists.txt with the name of the project and do cmake .. 
/// Then once we have the code, do make and then ./NAMEPROJECT to run it. 

/// PROJECT 2: DOCUMENT SCANNER

// Varible definitions

Mat imgOrg, imgGray, imgEdge, imgWarp, imgCrop; 
vector<Point> InitialPoints, FinalPoints;
float w = 420, h = 596;
int pixels = 5; // To define how many pixels we use to crop image.


Mat Preprocessing(Mat imgOrg){

    cvtColor(imgOrg,imgGray,COLOR_BGR2GRAY); // Data in opencv is read as BGR and not RGB, like in python. Unlike python, we write input image and destination image inside the function.
    GaussianBlur(imgGray,imgGray,Size(3,3),3,0); // Kernel size of Gaussian blur, sigma in x and y. 
    Canny(imgGray,imgEdge,90,100); // Canny edge detector. Is common practice to use it on softly blurred images. The other parameters are thresholds for edge detection. 
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3)); // You need to define a kernel shape for your dilation operation, which is rectangular. The bigger the size, the more it will dilate/erode.
    dilate(imgEdge, imgEdge, kernel);


    return imgEdge;

}

vector<Point> getContours(Mat img){  // We define a Point function because we will return a Point object.

    vector<vector<Point>> contours; // We define the data type of contour like so: contours is a vector of vectors, in which each vector is defined by two points. 
    vector<Vec4i> hierarchy; // Hierarchy, (we don't use here), is a vector of 4 integer values, in which C++ has its own datatype. 
    int area, maxArea;
    float perimeter;

    findContours(img,contours,hierarchy,RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    vector<vector<Point>> polygon_sides(contours.size()); // It can't be as large as contours length.  
    vector<Point> paperPoint; // We define the paper corner points.
    // We can also filter by area, let's say if area is above > 1000 is an object. Also, we wil draw a bounding box around the objects and detect amount of sides the polygon has. 

    for (int i=0;i<contours.size();i++){

        area = contourArea(contours[i]); // Calculates area of contour. 
        //cout << area << endl;
        
        string ObjectType;

        if (area > 1000){ // Filter by size.
            perimeter = arcLength(contours[i],true); // Calculates perimeetr of contour. The boolean is to tell if the shape is closed or not (true here)
            approxPolyDP(contours[i], polygon_sides[i],0.02*perimeter,true); // We calculate the amount of sides in the polygon (tha'ts why the output there in the second comand, the third amount not sure and 4th is same as above, if closed.)
            
            if (area > maxArea && polygon_sides[i].size()==4){ 
                maxArea = area;
                paperPoint = {polygon_sides[i][0],polygon_sides[i][1],polygon_sides[i][2],polygon_sides[i][3]};
                //drawContours(imgOrg,polygon_sides,i, Scalar(255,0,255),5); // -1 for drawing ALL contours, colour, thickness. 
                }
            
        

            }
        }

        return paperPoint;
    }


void drawPoints(vector<Point> Points, Mat img){

    for (int i=0;i<Points.size();i++){

        circle(img,Points[i],20, Scalar(0,255,0),FILLED); // Image, center point, size of circle, colour, thickness (here we can also instead of giving a value, we can say FILLED for full)

    }
}

vector<Point> OrderPoints(vector<Point> Points){

    vector<Point> FinalPoints;
    vector<int> SumPoints, SubtractPoints;

    for (int i=0; i<Points.size(); i++){

        SumPoints.push_back(Points[i].x+Points[i].y);
        SubtractPoints.push_back(Points[i].x-Points[i].y);

    }
    // We need to do them in order, since pushing back is like appending, so 0, 1, 2, 3 (TL (sum min),TR,BL,BR (sum max))
    FinalPoints.push_back(Points[min_element(SumPoints.begin(),SumPoints.end())-SumPoints.begin()]); // Element 0.  algorithm to find the iterator pointing to the minimum element in the range defined by SumPoints.begin() and SumPoints.end(). - SumPoints.begin(): This part subtracts the iterator pointing to the beginning of the vector (SumPoints.begin()) from the iterator obtained in step 1. The result is an integer representing the index of the minimum element in the vector.
    FinalPoints.push_back(Points[max_element(SubtractPoints.begin(),SubtractPoints.end())-SubtractPoints.begin()]); // Element 1
    FinalPoints.push_back(Points[min_element(SubtractPoints.begin(),SubtractPoints.end())-SubtractPoints.begin()]); // Element 2
    FinalPoints.push_back(Points[max_element(SumPoints.begin(),SumPoints.end())-SumPoints.begin()]); // Element 3

    return FinalPoints;

}

Mat WarpingImage(vector<Point> Points,Mat img, float w, float h){

    Mat Transform_mat;

    Point2f src[4] ={Points[0],Points[1],Points[2],Points[3]}; // This is a point object (2f for float since it will be needed after.
                                                               // Then you create the source points [4] for 4 points and we define them like so (all corners of the card).
    Point2f warped[4] ={{0.0f,0.0f},{w,0.0f},{0.0f,h},{w,h}}; // This is the points corresponding to the warped image (that is a top view projection). For 0 in order to be float, you need to type like that.

    Transform_mat = getPerspectiveTransform(src,warped); // Here you create a transformation matrix betweeen the initial and destination points. 
    warpPerspective(img,imgWarp,Transform_mat, Point(w,h)); // Here you apply this transformation with the warpPerspective function. You define a point defining the w,h of your warped image.


    return imgWarp;
}

Mat imageCrop(Mat img, int pixels){

    Rect ROI(pixels,pixels,w-pixels*2,h-pixels*2); // x,y,w,h We define a rectangle that we put in our image.
    imgCrop = img(ROI);

    return imgCrop;
}

int main()
{

    // Read image
    string path = "Trial.jpg"; // Define path.
    imgOrg = imread(path);

    // Preprocess data - convert to edge image.
    imgEdge = Preprocessing(imgOrg);

    // Get corner points from edge image and draw. 

    InitialPoints = getContours(imgEdge);

    FinalPoints = OrderPoints(InitialPoints); // It reorders points in a consistent manner - 0 (TL), 1 (TR), 2 (BL), 3 (BR)


    // Warp image

    imgWarp = WarpingImage(FinalPoints,imgOrg,w,h);
    drawPoints(FinalPoints, imgOrg);

    // Crop image to perfect the document display

    imgCrop = imageCrop(imgWarp,pixels);

    // Display images

    imshow("BGR Image", imgOrg);
    imshow("Warp Image",imgCrop);
    //imshow("Edge image", imgEdge);

    waitKey(0);
}