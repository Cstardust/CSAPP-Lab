#include "cachelab.h"
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>

#define FILE_NAME 100

int hit , miss , eviction;
// 高速缓存的结构可以用(S组,E行,B(块大小),m位address)来描述。
// 高速缓存的大小C = S*E*B Bytes，m表示主存地址的位数
//  address  t位tag s位set index b位block offset
//  S=2^s , t = m-s-b;
int S = 0;
int E = 0;
int B = 0;

// address 
int m = 64;  //  addr bits
int t = 0;  //  tag
int s = 0;  //  set index
int b = 0;  //  block set

char fileName[FILE_NAME];


//  LRU实现（LRU是一个组内的替换策略）
    //  timeList 记录访问时间 . 每个节点是caheLineLRU
    //  addr的index通过hash映射到某一个组
        //  通过遍历所有cacheLineLRU check its tag 来判断是否命中
            //  命中 hit
                //  更新 update 
            //  没命中
                //  sz = E
                    //  evict 驱逐(通过timelist) 需要delete吗?
                    //  insert 加入新的 malloc? 
                //  sz < E    
                    //  加入
//  本program的LRU，实现了S组的LRU策略
//  每组的LRU，实现了O(1)的驱逐和加入，（定位到组后）并没有实现O(1)的get
//  若要实现O(1)的get，则需要手写hash

//  每个组中的cacheLine  组成TimeList
typedef struct cacheLineLRU{
    // unsigned int valid;
    unsigned long tag;
    // int value;
    struct cacheLineLRU* prev,*next;
}cacheLineLRU;


//  每个组LRU cache策略的time list。总共有S个组，每个组E行;
typedef struct timeListLRU{
    int size;           //  就是E行cacheLine的E
    cacheLineLRU *head,*tail;
}timeListLRU;

//  s位index -> 组
timeListLRU* hashIndexToLRU;



void printUsage()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void parseOption(int argc,char *argv[])
{
    int op;
    while((op=getopt(argc,argv,"hvs:E:b:t:"))!=-1)
    {
        switch(op)
        {
            case 'h':
            {
                printUsage();
                break;   
            }
            case 'v':
            {
                printf("-v not implemented\n");
                break;
            }
            case 's':
            {
                // printf("-s argv : %s\n",optarg);
                s = atoi(optarg);
                S = 1 << s;
            }
            case 'E':
            {
                // printf("-E argv : %s\n",optarg);
                E = atoi(optarg);
            }
            case 'b':
            {
                // printf("-b argv : %s\n",optarg);
                b = atoi(optarg);
                B = 1<<b;
            }
            case 't':
            {
                strcpy(fileName,optarg);
                break;
            }
            default:
                printf("sth unknown : %s\n",optarg);
        }
    }
    // printf("S = %d , E = %d , B = %d , m = %d , t = %d , s = %d , b = %d , file = %s\n",S,E,B,m,t,s,b,fileName);
}


void initTimeListLRU(int i)
{
    //  list size
    hashIndexToLRU[i].size = 0;
    //  build list
    hashIndexToLRU[i].head = malloc(sizeof(cacheLineLRU));
    hashIndexToLRU[i].tail = malloc(sizeof(cacheLineLRU));
    hashIndexToLRU[i].head->next = hashIndexToLRU[i].tail;
    hashIndexToLRU[i].tail->prev = hashIndexToLRU[i].head;
}   



void removeFromTimeList(cacheLineLRU * t)
{
    t->next->prev = t->prev;
    t->prev->next = t->next;
}

void insertIntoTimeList(timeListLRU *LRU,cacheLineLRU *t)
{
    t->next = LRU->head->next;
    t->prev = LRU->head;
    LRU->head->next->prev = t;
    LRU->head->next = t;
    // printf("head->next=%p\t t=%p\t t->tag=%ld\t tail=%p\t t->next=%p\n",LRU->head->next,t,t->tag,LRU->tail,t->next);
}



void evictFromTimeList(timeListLRU *LRU)
{
    cacheLineLRU* t = LRU->tail->prev;
    LRU->tail->prev = t->prev;
    t->prev->next = LRU->tail;
    free(t);
}


//  由于不用store data , 也不用return data 因此 M,L,S都可合并为一个
void updateCache(unsigned long addr)
{
    // unsigned long mask = 0xffffffffffffffff;
    // // unsigned int block_offset =   不用管 因为不用处理data
    // unsigned long index = ((addr >> b)<<(m-s))>>(m-s);
    // printf("mask = %lx\n index = %ld\n",mask,index);

    //  如何获取一个数（称为待分解数）的低x位  (总位数：m) 
        //  先定义一个unsigned全f掩码
        //  然后利用unsigned特性(右移左侧补0)右移(m-x)位，然后得到b位全f掩码(高m-x位均为0)
        //  将待分解数与b位全f掩码相&，得到待分解数的低x位。
    unsigned long mask = 0xffffffffffffffff;
    // unsigned long blockOffetMask = mask >> (m-b);           //  由于mask为unsigned 故左侧补0
    // unsigned long blockOffset = blockOffetMask & addr;
    // printf("mask = %lx\tblockOffsetMask=%lx\tblockOffset=%ld\n",mask,blockOffetMask,blockOffset);

    unsigned long setIndexMask = mask >> (m-s);
    unsigned long setIndex = (addr >> b) & setIndexMask;
    // printf("mask = %lx\tsetIndexMask=%lx  \tsetIndex=%ld\n",mask,setIndexMask,setIndex);

    unsigned long tag = (addr >> (s+b));  // & (mask>>(s+b));  //  由于addr为unsigned。因此高位补0，没必要&
    // printf("tag=%ld\n",tag);


    unsigned isFound = 0;
    timeListLRU *LRU = hashIndexToLRU + setIndex;
    cacheLineLRU *dummy = LRU->head;
    cacheLineLRU *cur = dummy->next;
    while(cur!=LRU->tail)
    {
        if(cur->tag == tag){        // 行匹配 遍历 没做到 O(1)
            isFound = 1;
            break;
        }
        cur = cur->next;
    }    

    //  not need to deal with data  L : get data / S : insert data / M : load and insert 
    if(isFound){
        // printf("hit\n");
        //  update time list  
        removeFromTimeList(cur);
        insertIntoTimeList(LRU,cur);
        ++hit;
        return ;
    }else{
        ++miss;
        //  sz < capacity
        if((LRU->size) < E)        
        {
            // printf("miss\n");
            //  Add the New CacheLine(Node)
            cacheLineLRU *t = (cacheLineLRU *)malloc(sizeof(cacheLineLRU));
            t->tag = tag;
            insertIntoTimeList(LRU,t);
            ++(LRU->size);
        }
        else if(LRU->size == E)
        {
            // printf("miss evction tag=%ld\n",tag);
            //  Evict the victim
            --(LRU->size);
            ++eviction;
            evictFromTimeList(LRU);     //  delete
            //  Add the New CacheLine(Node)
            cacheLineLRU *t = (cacheLineLRU *)malloc(sizeof(cacheLineLRU));
            t->tag = tag;
            insertIntoTimeList(LRU,t);
            ++(LRU->size);
        }
        else
        {
            printf("LRU Internal Error\n");
        }
                
    }
}

void cacheSimulate()
{
    //  1. initCache
        //  1.1 S 组 timeList    
        //  1.2 每组的容量是E （也即capacity = E，链表上最多有E个节点） 这里是LRU
    hashIndexToLRU = (timeListLRU*)malloc(S * sizeof(timeListLRU));
    for(int i = 0;i < S; ++i)
        initTimeListLRU(i);

    FILE *f = fopen(fileName,"r");
    unsigned long addr;
    int sz;
    char op;
    while(fscanf(f," %c %lx,%d",&op,&addr,&sz)>0)
    {
        printf("%c, %lx %d\n",op,addr,sz);
        switch(op)
        {
            case 'L':       //  load data
                updateCache(addr);
                break;
            case 'M':       //  modified = load and store    load hit then update cache(write-back) . load not hit and then load into cache and update(write-allocate) 
                updateCache(addr);   //  load
            case 'S':       //  store
                updateCache(addr);
                break;             
        }
    }

}


int main(int argc,char *argv[])
{
    parseOption(argc,argv);
    
    cacheSimulate();

    printSummary(hit, miss, eviction);
    return 0;
}
