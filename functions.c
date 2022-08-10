#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <math.h>  
#include "functions.h"

//Reading pictures from file
Element* readPicturesFromFile(const char* fileName, double *matchingValue, int *numOfPictures) {
	FILE *fp;

	if ((fp = fopen(fileName, "r")) == 0){
		printf("cannot open file %s for reading\n", fileName);
		exit(0);
	}
	
	fscanf(fp, "%lf", matchingValue);
	//printf("%lf\n", *matchingValue);
	fscanf(fp, "%d", numOfPictures);
	//printf("%d\n", *numOfPictures);
	
	Element* pictures = (Element*)malloc((*numOfPictures) * sizeof(Element));
	if (pictures == NULL) {
		printf("Problem to allocate memory\n");
		exit(0);
	}
	
	for(int i = 0 ; i < *numOfPictures; i++)
	{

		fscanf(fp, "%d", &pictures[i].Id);
		//printf("%d\n", pictures[i].Id);

		fscanf(fp, "%d", &pictures[i].dimention);
		//printf("%d\n", pictures[i].dimention);

		pictures[i].members = (int*)malloc((pictures[i].dimention*pictures[i].dimention) * sizeof(int));
		
		for(int j = 0 ; j < (pictures[i].dimention*pictures[i].dimention) ; j++)
		{
			fscanf(fp, "%d", &pictures[i].members[j]);
			//printf("%d ", pictures[i].members[j]);

		}
		//printf)\n);
	}
	return pictures;
}

//Reading objects from file
Element** readObjectsFromFile(const char* fileName, int *numOfObjects) {
	FILE *fp;
	fscanf(fp, "%d", numOfObjects);
	//printf("%d\n", *numOfObjects);

		Element** objects = (Element**)malloc((*numOfObjects) * sizeof(Element*));
		if (objects == NULL){
			printf("Problem to allocate memory\n");
			exit(0);
		}

		for(int i = 0 ; i < *numOfObjects; i++){
			
			objects[i] = (Element*)malloc(1*sizeof(Element));

			fscanf(fp, "%d", &objects[i]->Id);
			fscanf(fp, "%d", &objects[i]->dimention);
			
			//printf("%d\n", objects[i]->Id);
			//printf("%d\n", objects[i]->dimention);

			objects[i]->members = (int*)malloc((objects[i]->dimention*objects[i]->dimention) * sizeof(int));
			for(int j = 0 ; j < (objects[i]->dimention*objects[i]->dimention) ; j++)
			{
				fscanf(fp, "%d", &objects[i]->members[j]);
				//printf("%d ", objects[i]->members[j]);
			}
			//printf)\n);
		}
		
	fclose(fp);
	return objects;
}
//method that sums all abs results
double calcMatchSum(Element* picture, Element* object, int i, int j,double matchingValue) {
	double absSum = 0;
	int column = j;
	for (int k = 0; k < object->dimention; k++, i++) {
		column = j;
		for (int x = 0; x < object->dimention; x++, column++) {

			absSum = absSum + fabs((double)((picture->members[i * picture->dimention + column] - object->members[k * object->dimention + x]) / (double)picture->members[i * picture->dimention+ column]));
			
			/*** If we will use thr condition below, the result will be better, but the function will not consider as an "heavy function". ***/
			
			//if(absSum>matchingValue){
			//	return absSum;
			//}
		}
	}
	return absSum;
}


//Main method that checks if picture has 'match' with object or not and puts the result in struct that will use later for printing result
void findPair(Element* picture, Element** object , double matchingValue, Pair* pair, int* Count, int numOfObjects)
{
	pair->i =-1;
	pair->j = -1;
	pair->pictureID = picture->Id;
	pair->objectID = -1;
	pair-> isPair = 0;
	
	int foundObj=0;
	int i;
	int j;
	double matcing=0;
	
	for(int obj =0; obj< numOfObjects; obj++){
		#pragma omp parallel private(matcing,i,j)
		{
			#pragma omp for	
			for(i =0; i< (picture->dimention - object[obj]->dimention+1) ; i++)
			{
				matcing=0;
				for(j=0; j< (picture->dimention - object[obj]->dimention+1) && !foundObj; j++)
				{	
					matcing = calcMatchSum(picture, object[obj], i, j, matchingValue);	
											
					if(matcing <= matchingValue && !foundObj) 
					{
						pair->i = i;
						pair->j = j;
						pair->pictureID = picture->Id;
						pair->objectID = object[obj]->Id;
						pair-> isPair = 1;
						foundObj=1;	

					}				
				}	
			}
		}		
	}
	
	*Count+=1;
}

//Method that sends data to slaves
void sendDataToSlaves(int num_procs,int numOfPictures, double matchingValue,int numOfObjects, Element* pictures,  Element** objects, int pictureToSend )
{
	int indexToSend = 0;
	int lastIndex;
	int flag=0;
	int finish;

		for(int i = 1; i< num_procs; i++)
		{
			if(i>numOfPictures) {
				lastIndex = i;
				flag = 1;
				break;
			}
			
			MPI_Send(&finish,0 ,MPI_INT,i,WORK,MPI_COMM_WORLD);
			MPI_Send(&matchingValue,1 ,MPI_DOUBLE,i,WORK,MPI_COMM_WORLD);
			MPI_Send(&numOfObjects,1 ,MPI_INT,i,WORK,MPI_COMM_WORLD);
			MPI_Send(&numOfPictures,1 ,MPI_INT,i,WORK,MPI_COMM_WORLD);
			MPI_Send(&pictureToSend,1 ,MPI_INT,i,WORK,MPI_COMM_WORLD);

			for(int x=0; x < pictureToSend; x++)
			{
				MPI_Send(&pictures[indexToSend].dimention, 1, MPI_INT, i, WORK, MPI_COMM_WORLD);
				MPI_Send(&pictures[indexToSend].Id, 1, MPI_INT, i, WORK, MPI_COMM_WORLD);
					
				for (int j = 0; j < (pictures[indexToSend].dimention*pictures[indexToSend].dimention); j++)
				{
						MPI_Send(&pictures[indexToSend].members[j], 1, MPI_INT, i, WORK, MPI_COMM_WORLD);	
						//printf("%d ",pictures[indexToSend].members[j]);
				}
				//printf("\n");
				indexToSend++;
			}
				
			for(int x=0; x < numOfObjects; x++)
			{
				MPI_Send(&objects[x]->dimention, 1, MPI_INT, i, WORK, MPI_COMM_WORLD);
				MPI_Send(&objects[x]->Id, 1, MPI_INT, i, WORK, MPI_COMM_WORLD);

				for (int j = 0; j < (objects[x]->dimention*objects[x]->dimention); j++)
				{
						MPI_Send(&objects[x]->members[j], 1, MPI_INT, i, WORK, MPI_COMM_WORLD);			
						//printf("%d ",objects[x]->members[j]);
				}
				//printf("\n");
			}
		}
			
			for(int i = lastIndex; i< num_procs && flag; i++)
				MPI_Send(&finish, 0, MPI_INT, i, STOP, MPI_COMM_WORLD);		
}

//Method that recvs data from prosses 0
void recvDataFromMaster(int num_procs ,Element*** ranksPicture, Element*** ranksObjects,int* numOfObjects,int* numOfPictures , double* matchingValue, int* pictureToRecv)
{
	MPI_Status status;
	
	MPI_Recv(matchingValue,1,MPI_DOUBLE,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	MPI_Recv(numOfObjects,1 ,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	MPI_Recv(numOfPictures,1 ,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	MPI_Recv(pictureToRecv,1 ,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	
	*ranksPicture = (Element**)malloc(*pictureToRecv*sizeof(Element*));
		if (*ranksPicture == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
		
	Element** picTemp = *ranksPicture;

	for(int j =0; j< *pictureToRecv; j++)
	{
			   
		picTemp[j] = (Element*)malloc(*pictureToRecv*sizeof(Element));
		if (picTemp[j] == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
			
		MPI_Recv(&picTemp[j]->dimention, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//printf("dim :%d\n",picTemp[j]->dimention);
			
		MPI_Recv(&picTemp[j]->Id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//printf("ID :%d\n" ,picTemp[j]->Id);

		picTemp[j]->members = (int*)malloc((picTemp[j]->dimention*picTemp[j]->dimention)*sizeof(int));
		if (picTemp[j]->members == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
			
		for (int i = 0; i < (picTemp[j]->dimention*picTemp[j]->dimention); i++)
		{

			MPI_Recv(&picTemp[j]->members[i], 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
           			// printf("%d ",picTemp[j]->members[i]);
		}
			//printf("\n");
	}

	*ranksObjects = (Element**)malloc((*numOfObjects)*sizeof(Element*));
	if (*ranksObjects == NULL) {
		printf("Problem to allocate memory\n");
		exit(0);
	}
		
	Element** objTemp = *ranksObjects;
		
	for(int j =0; j<*numOfObjects; j++)
	{
		objTemp[j] = (Element*)malloc(*numOfObjects*sizeof(Element));
		if (objTemp[j] == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
			
		MPI_Recv(&objTemp[j]->dimention, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//printf("dim :%d\n",objTemp[j]->dimention);
			
		MPI_Recv(&objTemp[j]->Id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//printf("ID :%d\n" ,objTemp[j]->Id);
			
			
		objTemp[j]->members = (int*)malloc((objTemp[j]->dimention*objTemp[j]->dimention)*sizeof(int));
		if (objTemp[j]->members == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
			
		for (int i = 0; i < (objTemp[j]->dimention*objTemp[j]->dimention); i++)
		{
			
				MPI_Recv(&objTemp[j]->members[i], 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				//printf("%d ",objTemp[j]->members[i]);
		}
		//printf("\n");
	}	
}

//Method that prints final result to output.txt (if using make file)
void printResult(int totalCount, Pair* finalResult, Element* pictures)
{
	for(int i =0; i < totalCount; i++)
	{
		if(finalResult[i].isPair == 1)	
			printf("picture %d found Object %d in  Position (%d, %d)\n",finalResult[i].pictureID ,finalResult[i].objectID ,finalResult[i].i,finalResult[i].j);
		else	
			printf("picture %d No Objects were found\n", finalResult[i].pictureID );
	}
}

	
