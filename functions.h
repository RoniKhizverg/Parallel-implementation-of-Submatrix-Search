#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#define FILE_NAME "input.txt"
#define ROOT 0
enum tags {WORK, STOP};


typedef struct {
	int Id;
	int dimention;
	int* members;

}Element;

typedef struct {
	int i;
	int j;
	int pictureID;
	int objectID;
	int isPair;

}Pair;

Element* readPicturesFromFile(const char* fileName, double *matchingValue, int *numOfPictures);
Element** readObjectsFromFile(const char* fileName, int *numOfObjects);
double calcMatchSum(Element* picture, Element* object, int i, int j,double matchingValue);
void findPair(Element* picture, Element** object , double matchingValue, Pair* found, int* Count, int numOfObjects);
void sendDataToSlaves(int num_procs,int numOfPictures, double matchingValue,int numOfObjects, Element* pictures,  Element** objects, int pictureToSend );
void recvDataFromMaster(int num_procs ,Element*** ranksPicture, Element*** ranksObjects,int* numOfObjects,int* numOfPictures , double* matchingValue, int* pictureToRecv);
void printResult(int totalCount, Pair* finalResualt, Element* pictures);

#endif
