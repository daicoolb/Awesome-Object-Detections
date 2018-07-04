#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
 
using namespace std;
using namespace cv;
//使用感知哈希算法进行图片去重
//构建一个结构体，用于存储对应名称和要比较的内容
typedef struct MatStruct{
    string name;
    unsigned char buf[64];
    MatStruct(){
        name = "";
        memset(buf, 0, 64);
    }
    MatStruct(const struct MatStruct &ms){
        name = ms.name;
        memset(buf, 0, 64);
        memcpy(buf, ms.buf, 64);
    }
    //重载比较函数
    bool operator<(const struct MatStruct &ms)const {
        return name < ms.name;
    }
}MatStruct;
 
 
int getdirimages(string path, std::vector<MatStruct> &images)
{
    std::vector<MatStruct> tmpimages;
    DIR *dir;
    struct dirent *ptr;
    dir = opendir(path.c_str());
    while((ptr = readdir(dir)) != NULL) {
        if((strcmp(ptr->d_name,".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
            continue;
        }
        string fullname = path + "/" + ptr->d_name;
        Mat img = imread(fullname);
        if(img.empty()){
            continue;
        }
        MatStruct ms;
        ms.name = ptr->d_name;
        Mat gray, res;
        //缩放成8x8大小灰度图
        resize(img, res, Size(8,8));
        cvtColor(res, gray, CV_BGR2GRAY);
        //获取灰度平均值
        double mn = mean(gray)[0];
        //比较像素灰度，获取图像指纹
        for(int i = 0; i < 8; i++){
            for(int j = 0; j < 8; j++){
                ms.buf[i*8 + j] = (gray.at<unsigned char>(i,j) > mn) ? 1 : 0;
            }
        }
        cout << "get " << fullname << endl;
        tmpimages.push_back(ms);
    }
    closedir(dir);
    std::sort(tmpimages.begin(), tmpimages.end());
    tmpimages.swap(images);
}
 
 
int main(int argc, char* argv[])
{
    if(argc <= 1){
        cout << argv[0] << " picpath" << endl;
        return -1;
    }
    
    struct stat st;
    if(stat(argv[1], &st) != 0 || (st.st_mode & S_IFDIR) != S_IFDIR){
        cout << argv[1] << " is not dir" << endl;
        return -1;
    }
    
    int startpos = 0;
    int curpos = 0;
    std::vector<MatStruct> images;
    cout << "start getdir" << endl;
    getdirimages(argv[1], images);
    
    cout << "start compare" << endl;
    std::vector<MatStruct>::iterator it1 = images.begin();
    while(it1 != images.end()){
        std::vector<MatStruct>::iterator it2 = it1;
        curpos = 0;
        //存储需要删除的迭代对象
        std::vector<std::vector<MatStruct>::iterator> delImages;
        cout << it1->name << endl;
        while(it2 != images.end()){
            int diff = 0;
            if(it2 == it1){
                it2++;
                continue;
            }
            //比较两个图片的相似度，不同的地方不超过5，则为相似的图片
            for(int i = 0; i < 64; i++){
                if(it1->buf[i] != it2->buf[i]){
                    diff++;
                }
            }
            if(diff < 5){
               delImages.push_back(it2);
            }
            it2++;
        }
        if(delImages.size() > 0){
            //删除图像
            for(int i = 0; i < delImages.size(); i++){
                cout << "remove " << delImages[i]->name << endl;
                string fullpath = string(argv[1]) + "/" + delImages[i]->name;
                unlink(fullpath.c_str());
                images.erase(delImages[i]);
            }
            it1 = images.begin();
            for(int index = 0; index < startpos; index++)
                it1++;
            continue;
        }
        startpos++;
        it1++;
    }
    return 0;
}
