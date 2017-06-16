//
//  main.c
//  OS
//
//  Created by Croff on 2017/4/25.
//  Copyright © 2017年 Croff. All rights reserved.
//

//文件名仅支持ASCII码
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLENGTH 64   //最大字符串长度为64
#define MAXFILENUM 224  //最大文件数为224

typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

#pragma pack (1) 
//指定按1字节对齐

//偏移11个字节
struct BPB {
    u16 BPB_BytsPerSec; //每扇区字节数
    u8  BPB_SecPerClus; //每簇扇区数
    u16 BPB_RsvdSecCnt; //Boot记录占用的扇区数
    u8  BPB_NumFATs;    //FAT表个数
    u16 BPB_RootEntCnt; //根目录最大文件数
    u16 BPB_TotSec16;   //扇区总数
    u8  BPB_Media;      //介质描述符
    u16 BPB_FATSz16;    //FAT扇区数
    u16 BPB_SecPerTrk;  //每个磁道的扇区数
    u16 BPB_NumHeads;   //磁头数（扇面数）
    u32 BPB_HiddSec;    //隐藏扇区数
    u32 BPB_TotSec32;   //如果BPB_FATSz16为0，该值为FAT扇区数
};
//BPB结束，长度25字节

//根目录条目
struct RootEntry {
    char DIR_Name[11];   //文件名8字节，扩展名3字节
    u8   DIR_Attr;       //文件属性
    u8   reserved[10];   //保留位10字节
    u16  DIR_WrtTime;    //最后一次写入时间
    u16  DIR_WrtDate;    //最后一次写入日期
    u16  DIR_FstClus;    //文件的开始簇号
    u32  DIR_FileSize;   //文件的大小
};
//根目录条目结束，32字节

#pragma pack ()
//取消指定对齐，恢复缺省对齐

struct File {
    char fileName[MAXLENGTH];
    int startClus;
    int isFile;
};

int  BytsPerSec;    //每扇区字节数
int  SecPerClus;    //每簇扇区数
int  RsvdSecCnt;    //Boot记录占用的扇区数
int  NumFATs;       //FAT表个数
int  RootEntCnt;    //根目录最大文件数
int  FATSz;         //FAT扇区数

struct File files[MAXFILENUM];  //文件列表
int  fileNum = 0;   //文件数量

void getInput(char* input);  //获取输入的命令
void readBPB(FILE* fat12, struct BPB* bpb_ptr); //读取文件信息，将信息填充到BPB中
void printFileNames(FILE* fat12);   //打印文件名
void printDirectory(FILE* fat12, char* directory, int startClus);   //打印目录及目录下子文件名
u16  getFATValue(FILE* fat12, int num); //读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值
int  illegalFileName(char fileName[]);  //判断是否是合法文件名
void getFileName(char name[], char fileName[]); //提取fileName中的文件名和后缀名到name中
void getDirName(char name[], char dirName[]);   //提取dirName中的文件夹名到name中
void recordFile(char fileName[], int startClus, int isFile);    //把所有的文件名（包括空目录名都放进file[]集合中）

struct File* getFileInfo(char fileName[]);  //根据文件名从保存的文件列表获取文件信息
void printFileContent(FILE* fat12, int startClus);  //打印文件内容
void printDirectoryContent(char directory[]);   //打印文件夹内容
void countDirectory(char directory[], int tabNum);   //count命令

extern void printStr(char* str, int len);	//nasm打印普通字符串
extern void printFileName(char* str, int len);	//nasm打印文件名
extern void printFileCatalog(char* str, int len);	//nasm打印目录名

void printStrByNasm(char* str);	//打印普通字符串
void printFileNameByNasm(char* str);		//打印文件名
void printFileCatalogByNasm(char* str);	//打印目录名

int main() {
    //打开FAT12的映像文件
    FILE* fat12 = fopen("a.img", "rb");
    if(fat12 == NULL) {
        //文件a.img未找到
        printStrByNasm("File 'a.img' not found.\n");
        exit(0);
    }
    
    struct BPB bpb;
    struct BPB* bpb_ptr = &bpb;
    //读取文件信息，将信息填充到BPB中
    readBPB(fat12, bpb_ptr);
    
    //初始化各个全局变量
    BytsPerSec = bpb_ptr->BPB_BytsPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    FATSz = bpb_ptr->BPB_FATSz16;
    if(FATSz == 0){
        FATSz = bpb_ptr->BPB_TotSec32;
    }
    
    //打印所有文件及其路径
    printFileNames(fat12);
    
    //显示用户输入命令的提示并接受和执行用户的命令
    char exit[] = "exit";
    char count[] = "count";
    printStrByNasm("\nCommands:\n");
    printStrByNasm("1.locate path and print content\n");
    printStrByNasm("2.count directory\n");
    printStrByNasm("3.exit\n");
    printStrByNasm("Please input command: ");
	
	char input[MAXLENGTH];
	getInput(input);
    while(strcasecmp(input, exit) != 0) {
        if(strncasecmp(input, count, 5) == 0) {
            //count指令
			getInput(input);
            char txtSuffix[] = "TXT";
            char fileSuffix[4];
            int length = (int)strlen(input);
            int i = 0;
            for(; i<3; i++) {
                fileSuffix[i]=input[length-1-i];
            }
            fileSuffix[i]='\0';
            //获取输入的文件名后缀名
            if(strcasecmp(fileSuffix, txtSuffix) == 0) {
                printStrByNasm("Not a directory.\n");
            } else {
                countDirectory(input, 0);
            }
        } else {
            //读内容指令
            struct File* file_ptr = getFileInfo(input);
            if(file_ptr != NULL && file_ptr->isFile == 1) {
                //打印文件内容
                printFileContent(fat12, file_ptr->startClus);
            } else {
                //打印目录内容
                printDirectoryContent(input);
            }
        }
        
        //接受下一条指令
        printStrByNasm("\nPlease input command: ");
        getInput(input);
    }
    
    fclose(fat12);
    return 0;
}

void getInput(char* input) {
    int index;
    //清空input
    for(index = 0; index < MAXLENGTH; index++) {
        input[index] = '\0';
    }
    
    scanf("%s", input);
}

void readBPB(FILE* fat12, struct BPB* bpb_ptr) {
    
    /*
     BPB从偏移11个字节处开始
     int fseek(FILE *stream, long offset, int fromwhere)
     fseek函数设置文件指针stream的位置
     如果执行成功，stream将指向以fromwhere为基准，偏移offset（指针偏移量）个字节的位置，函数返回0
     如果执行失败(比如offset超过文件自身大小)，则不改变stream指向的位置，函数返回一个非0值
     偏移起始位置：文件头0(SEEK_SET)，当前位置1(SEEK_CUR)，文件尾2(SEEK_END)
     */
    int check = (int)fseek(fat12, 11, SEEK_SET);
    if (check != 0) {
        printStrByNasm("fseek in readBPB failed!\n");
        exit(0);
    }
    
    /*
     BPB长度为25字节
     int fread(void* buffer, int size, int count, FILE* stream)
     fread从一个文件流中读数据，最多读取count个项，每个项size个字节
     buffer：用于接收数据的内存地址
     size：要读的每个数据项的字节数，单位是字节
     count：要读count个数据项，每个数据项size个字节.
     stream：输入流
     如果调用成功返回实际读取到的项个数（小于或等于count）
     如果不成功或读到文件末尾返回 0
     */
    check = (int)fread(bpb_ptr, 1, 25, fat12);
    if (check != 25) {
        printStrByNasm("fread in readBPB failed!\n");
        exit(0);
    }
}

void printFileNames(FILE* fat12) {
    //base指向根目录开头
    int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
    
    //rootEntry_ptr每次指向一个根目录条目
    struct RootEntry rootEntry;
    struct RootEntry* rootEntry_ptr = &rootEntry;
    
    int i = 0;
    while(i < RootEntCnt) {
        fseek(fat12, base + i * 32, SEEK_SET);
        fread(rootEntry_ptr, 1, 32, fat12);
        
        if(illegalFileName(rootEntry_ptr->DIR_Name) == 1){
            i++;
            continue;
        }
        
        if(rootEntry_ptr->DIR_Attr == 0x10) {
            //DIR_Attr表示文件属性，为0x10表示目录
            char dir[MAXLENGTH];
            getDirName(dir, rootEntry_ptr->DIR_Name);
            printDirectory(fat12, dir, rootEntry_ptr->DIR_FstClus);
        } else {
            //否则表示文件
            char name[MAXLENGTH];
            getFileName(name, rootEntry_ptr->DIR_Name);
            printFileNameByNasm(name);
            printStrByNasm("\n");
            recordFile(name, rootEntry_ptr->DIR_FstClus, 1);
        }
        i++;
    }
}

int illegalFileName(char fileName[]) {
    //文件名不能为空
    if(fileName[0] == '\0') {
        return 1;
    }
    
    //文件名只允许数字或字母
    int index;
    for(index=0; index<11; index++) {
        if(!(((fileName[index] >= 48)&&(fileName[index] <= 57)) ||
             ((fileName[index] >= 65)&&(fileName[index] <= 90)) ||
             ((fileName[index] >= 97)&&(fileName[index] <= 122)) ||
             (fileName[index] == ' '))) {
            return 1;
        }
    }
    
    return 0;
}

void getDirName(char name[], char dirName[]) {
    int i, k = 0;
    for(i=0; i<11; i++) {
        if(dirName[i] != ' ') {
            name[k++] = dirName[i];
        } else {
            name[k] = '\0';
            break;
        }
    }
}

void printDirectory(FILE* fat12, char* directory, int startClus) {
    int base = (RsvdSecCnt+NumFATs*FATSz+(RootEntCnt*32+BytsPerSec-1)/BytsPerSec)*BytsPerSec;
    //base会指向数据区起始地址
    //directory用于累加目录的名字
    int isEmptyDir = 1;
    
    struct RootEntry rootEntry;
    struct RootEntry* rootEntry_ptr = &rootEntry;
    
    int bytsPerClus = SecPerClus * BytsPerSec;
    int FATValue = 0;
    int currentClus = startClus;
    //当前簇号，根据FAT表中的值变化
    
    while(FATValue < 0xFF7) {
        FATValue = getFATValue(fat12, startClus);
        //FATValue表示本文件下一个簇的簇号
        base += (currentClus - 2) * bytsPerClus;
        
        int i = 0;
        while(i < bytsPerClus) {
            //读一个簇的内容
            fseek(fat12, base + i, SEEK_SET);
            fread(rootEntry_ptr, 1, 32, fat12);
            //读32个数据项，每个项一个字节，读到根目录中
            currentClus = FATValue;
            
            if(illegalFileName(rootEntry_ptr->DIR_Name) == 1) {
                i += 32;
                continue;
            } else {
                isEmptyDir = 0;
            }
            
            if(rootEntry_ptr->DIR_Attr != 0x10) {
                //无子目录
                char fullName[MAXLENGTH];
                char parent[MAXLENGTH];
                strcpy(fullName, directory);
                int len = (int)strlen(directory);
                fullName[len] = '/';
                fullName[len+1] = '\0';
                strcpy(parent, fullName);
                char name[MAXLENGTH];
                getFileName(name, rootEntry_ptr->DIR_Name);
                strcat(fullName, name);
                printFileCatalogByNasm(parent);
                printFileNameByNasm(name);
                printStrByNasm("\n");
                
                recordFile(fullName, rootEntry_ptr->DIR_FstClus, 1);
            } else {
                //仍有子目录
                char parent[MAXLENGTH];
                strcpy(parent, directory);
                int len = (int)strlen(directory);
                parent[len] = '/';
                parent[len+1] = '\0';
                char dir[MAXLENGTH];
                getDirName(dir, rootEntry_ptr->DIR_Name);
                strcat(parent, dir);
                printDirectory(fat12, parent, rootEntry_ptr->DIR_FstClus);
            }
            
            //指向下一个数据项
            i += 32;
        }
        
    }
    
    //空目录直接输出
    if(isEmptyDir == 1) {
		strcat(directory, "/");
		printFileCatalogByNasm(directory);
		printStrByNasm("\n");
        recordFile(directory, startClus, 0);
    }
}

void getFileName(char name[], char fileName[]) {
    //DIR_Name共11位，长度有文件名8位，后3位是扩展名
    int index1, index2 = 0;
    int fullLength = 1;
    //fullLength表示11位文件名中是否不存在空格
    
    for(index1 = 0; index1 < 11; index1 ++) {
        if(fileName[index1] == ' ') {
            fullLength = 0;
            break;
        }
    }
    
    //11位文件名中不存在空格
    if(fullLength == 1) {
        for(index1 = 0; index1 < 8; index1 ++) {
            name[index1] = fileName[index1];
        }
        name[index1] = '.';
        for(; index1 < 11; index1 ++) {
            name[index1+1] = fileName[index1];
        }
        name[index1+1] = '\0';
        
        return;
    }
    
    //11位文件名中存在空格
    for(index1=0; index1<11; index1++) {
        if(fileName[index1] != ' ') {
            name[index2++] = fileName[index1];
        } else {
            name[index2++] = '.';
            while(fileName[index1] == ' ') {
                index1++;
            }
            index1--;
        }
    }
    name[index2] = '\0';
}

void recordFile(char fileName[], int startClus, int isFile) {
    strcpy(files[fileNum].fileName, fileName);
    files[fileNum].startClus = startClus;
    files[fileNum].isFile = isFile;
    fileNum++;
}

u16 getFATValue(FILE* fat12, int startClus) {
    //FAT1的基址
    int base = RsvdSecCnt * BytsPerSec;
    //FAT项的偏移字节
    int offset = base + startClus * 3/2;
    //奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
    int type = 0;
    if (startClus % 2 == 0) {
        type = 0;
    } else {
        type = 1;
    }
    
    //先读出FAT项所在的两个字节
    u16 bytes;
    u16* bytes_ptr = &bytes;
    int check;
    check = fseek(fat12, offset, SEEK_SET);
    if (check == -1) {
        printStrByNasm("fseek in getFATValue failed!\n");
    }
    
    check = (int)fread(bytes_ptr, 1, 2, fat12);
    if (check != 2) {
        printStrByNasm("fread in getFATValue failed!\n");
    }
    
    //u16为unsigned short，结合存储的小尾顺序和FAT项结构可以得到
    //type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
    if (type == 0) {
        bytes = bytes << 4;
        return bytes>>4;
    } else {
        return bytes>>4;
    }
}

struct File* getFileInfo(char fileName[]) {
    int i;
    for(i=0; i<fileNum; i++) {
        if(strcasecmp(files[i].fileName, fileName) == 0) {
            if(files[i].isFile == 1){
                return &files[i];
            } else {
                break;
            }
        }
    }
    return NULL;
}

void printFileContent(FILE* fat12, int startClus) {
    int base = (RsvdSecCnt+NumFATs*FATSz+(RootEntCnt*32+BytsPerSec-1)/BytsPerSec)*BytsPerSec;
    //base指向数据区
    char* txt = malloc(BytsPerSec*5);
    //文件全部内容，容量为5个扇区
    int currentClus = startClus;
    //数据偏移的簇号
    while (currentClus < 0xFF7) {
        int offset = base + (currentClus - 2) * BytsPerSec;
        char *content = malloc(BytsPerSec);
        fseek(fat12, offset, SEEK_SET);
        fread(content, BytsPerSec, 1, fat12);
        strcat(txt, content);
        //检查FAT表
        currentClus = getFATValue(fat12, currentClus);
    }
    
    if(currentClus == 0xFF7) {
        //当前簇为坏簇
        printStrByNasm("Bad Cluster!\n");
    } else {
        //当前簇为最后一个簇
        printStrByNasm(txt);
    }
}

void printDirectoryContent(char directory[]) {
    int unknownFile = 1;
    int i;
    for(i=0; i<fileNum; i++) {
        if(strcasecmp(files[i].fileName, directory) == 0) {
            //目录为空时打印Empty directory.
            printStrByNasm("Empty directory.\n");
            unknownFile = 0;
            break;
        } else if(strncasecmp(files[i].fileName, directory, strlen(directory)) == 0) {
            //目录不为空时打印目录下所有文件
            printStrByNasm(files[i].fileName);
            printStrByNasm("\n");
            unknownFile = 0;
        }
    }
    
    if(unknownFile == 1) {
        printStrByNasm("Unknown file.\n");
    }
}

void countDirectory(char directory[], int tabNum) {
	char countedDir[MAXLENGTH][MAXLENGTH];
	int i, j, k;
	int isExsit = 0;
	
	//存储已记录过的目录名
    int countDir = 0;
	int countFile = 0;
    for(i=0; i<fileNum; i++) {
        if(strncasecmp(files[i].fileName, directory, strlen(directory)) == 0) {
			isExsit = 1;
			int isDirectory = 0;
			j = (int)strlen(directory) + 1;
			while (j < strlen(files[i].fileName)) {
				if(files[i].fileName[j] == '/') {
					//子目录
					char childDir[MAXLENGTH];
					strncpy(childDir, files[i].fileName, j);
					childDir[j] = '\0';
					
					int ifCounted = 0;
					for(k=0; k<countDir; k++) {
						//检查该目录是否记录过
						if(strcasecmp(countedDir[k], childDir) == 0) {
							ifCounted = 1;
							break;
						}
					}
					
					if(!ifCounted) {
						//没记录过就记录在countedDir中
						strcpy(countedDir[countDir], childDir);
						countDir++;
					}
					isDirectory = 1;
					break;
				}
				
				if(files[i].fileName[j] == '.') {
					//子文件
					countFile++;
					break;
				}
				
				j++;
			}
		}
    }
	
	if(isExsit) {
		char num1[10];
		char num2[10];
			
		sprintf(num1, "%d", countDir);
		sprintf(num2, "%d", countFile);
			
		for(i=0; i<tabNum; i++) {
			printStrByNasm("   ");
		}
		
		//以下代码为去除父文件名的操作
		char dirName[MAXLENGTH];
		int length = (int)strlen(directory);
		int dirNameLength;
		for(dirNameLength = 0; dirNameLength < length; dirNameLength++) {
			if(directory[length-1-dirNameLength] == '/') {
				break;
			}
		}
		
		k = 0;
		for(i=length-dirNameLength; i<length; i++) {
			dirName[k] = directory[i];
			k++;
		}
		dirName[k] = '\0';
		
		printStrByNasm(dirName);
		printStrByNasm(": ");
		printStrByNasm(num1);
		printStrByNasm(" directory, ");
		printStrByNasm(num2);
		printStrByNasm(" file.\n");
		
		//依次count子目录
		for(i=0; i<countDir; i++) {
			countDirectory(countedDir[i], tabNum+1);
		}
	} else {
		//文件不存在的情况
		printStrByNasm("Not a directory!\n");
	}
}

void printStrByNasm(char* str) {
	printStr(str, strlen(str));
}

void printFileNameByNasm(char* str) {
	printFileName(str, strlen(str));
}

void printFileCatalogByNasm(char* str) {
	printFileCatalog(str, strlen(str));
}