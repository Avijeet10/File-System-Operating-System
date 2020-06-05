/*
 Student Name: Avijeet Adhikari
 */

// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments
#define  BLOCK_SIZE 8192
#define  NUM_BLOCKS 4226
#define  NUM_FILES  128


FILE *fd;

uint8_t  blocks [NUM_BLOCKS] [BLOCK_SIZE];
void createfs  (char * filename);

struct Directory_Entry
{
    int8_t valid;
    char  name[32];
    uint32_t inode;
};

struct Inode
{
    time_t date;
    uint8_t valid;
    int8_t attributes;
    uint32_t size;
    uint32_t blocks[1250];
};

struct Directory_Entry * dir;
struct Inode           * inodeList;
uint8_t                * freeBlockList;
uint8_t                * freeInodeList;

void initializeBlockList()
{
    int i;
    for(i = 0; i < 4226; i++)
    {
        freeBlockList[i] = 1;
    }
}


void initializeInodeList()
{
    int i;
    for(i = 0; i < 128; i++)
    {
        freeInodeList[i] = 1;
    }
}


int findFreeInode()
{
    int ret = -1;
    int i;
    
    for(i = 0; i < 128; i++)
    {
        if(freeInodeList[i] == 1)
        {
            ret = i;
            freeInodeList[i] = 0;
            break;
        }
    }
    return ret;
}

int findFreeBlock()
{
    int ret = -1;
    int i;
    
    for (i = 132; i<NUM_BLOCKS; i++)
    {
        if(freeBlockList[i] == 1)
        {
            ret = i;
            freeBlockList[i] = 0;
            break;
        }
    }
    return ret;
}

int findFreeDirectory()
{
    int ret = -1;
    int i;
    
    for (i = 0; i<128; i++)
    {
        if(dir[i].valid == 0)
        {
            ret = i;
            dir[i].valid = 1;
            break;
        }
    }
    
    return ret;
}

void initializeDirectory()
{
    int i;
    for(i = 0; i < 128; i++)
    {
        dir[i].valid = 0;
        
        memset( dir[i].name,0, 32);
        
        dir[i].inode = -1;
    }
}

void initializeInodes()
{
    int i;
    for(i = 0; i< 128; i++)
    {
        int j;
        inodeList[i].attributes = 0;
        inodeList[i].size       = 0;
        inodeList[i].valid      = 0;
        for (j = 0; j<1250; j++)
        {
            inodeList[i].blocks[j] = -1;
            
        }
    }
}

void createfs  (char * filename)
{
    int i; 
    memset(&blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);
    
    fd = fopen (filename ,"w");
    if(fd==NULL)
    {
        printf("creaters: File not found");
    }  
   
    fwrite(&blocks[0], BLOCK_SIZE, NUM_BLOCKS, fd);
                
    fclose( fd);               
                
}

void open( char *filename)
{
    fd  = fopen (filename,"r");
    if(fd==NULL)
    {
        printf("open: File not found\n");
    }               
    fread(&blocks[0], BLOCK_SIZE, NUM_BLOCKS, fd);
    fclose(fd);                 
}

void closef(char *filename)
{
    fd=fopen(filename,"w");
    fwrite(&blocks[0],BLOCK_SIZE, NUM_BLOCKS, fd);
    fclose(fd);
}

int df()
{
    int i=0,count=0;
    int newsize;
    for(i=10;i<BLOCK_SIZE;i++)
    {
        if(freeBlockList[i] == 1)
        {
            count++;
        }
    }
    //printf("%d\n",count);
    newsize=count*BLOCK_SIZE;
    return newsize;
}
void list()
{
    int i;
    struct tm *tm_date;
    char date[32];
    int check_file=0;
    for(i= 0; i< NUM_FILES; i++)
    {
        if( dir[i].valid == 1 && inodeList[dir[i].inode].attributes!=-1)
        {
            tm_date = localtime(&inodeList[dir[i].inode].date);
            strftime(date,32,"%b  %d %H:%M", tm_date);
            printf("%d %s %s\n", inodeList[dir[i].inode].size, date, dir[i].name);
            check_file++;
        }
        
    }
    //if there is no presence of files, then above loop will not iterate
    //so no iteration (counting of check is 0) tells no presence of file
    if(check_file==0)
    {
        printf("list: No files found\n");
    }
    

    
}

void put(char* filename)
{
    
    struct stat buf;  
    int directory_index;               // stat struct to hold the returns from the stat call
    
    /*check if the file is exist*/ 
    
    if(stat(filename,&buf)==-1)
    {
        printf("put error: File doesnot exist\n");
        return;
    }
    
    if(strlen(filename)>32)
    {
        printf("put error: File name too long.\n");
        return ;
    }
    
    //printf("%d\n", buf.st_size);
    if(buf.st_size>df())
    {
        printf("put error: Not enough disk space.\n");
        return ;
    }
    directory_index=findFreeDirectory();
    //printf("%d\n",block_index);
    if(directory_index==-1)
    {
        printf("put error: No FreeDirectory.\n");
        return; 
    }
    dir[directory_index].valid = 1;
    strcpy(dir[directory_index].name,filename);

    int inode_index=findFreeInode();
   // printf("%d\n",inode_index);
    if(inode_index==-1)
    {
        printf("put error: No Free INode.\n");
        return; 
    }
    dir[directory_index].inode = inode_index;
    inodeList[inode_index].valid = 1;
    inodeList[inode_index].attributes = 0;        
    inodeList[inode_index].size = buf.st_size;
    inodeList[inode_index].date = time(NULL);
    
    
    int free_block_index=findFreeBlock();
    if(free_block_index==-1)
    {
        printf("put error: No Free block.\n");
        return; 
    }
    int inodeindex = 0;
    
    // Open the input file read-only 
    FILE *ifp = fopen ( filename, "r" ); 
    //printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );
 
    // Save off the size of the input file since we'll use it in a couple of places and 
    // also initialize our index variables to zero. 
    int copy_size   = buf . st_size;

    // We want to copy and write in chunks of BLOCK_SIZE. So to do this 
    // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
    // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
    int offset      = 0;               

    // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big 
    // memory pool. Why? We are simulating the way the file system stores file data in
    // blocks of space on the disk. block_index will keep us pointing to the area of
    // the area that we will read from or write to.
    
    //int block_index = 0;
 
    // copy_size is initialized to the size of the input file so each loop iteration we
    // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
    // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
    // we have copied all the data from the input file.
    while( copy_size > 0 )
    {
        inodeList[inode_index].blocks[inodeindex] = free_block_index;
      // Index into the input file by offset number of bytes.  Initially offset is set to
      // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
      // then increase the offset by BLOCK_SIZE and continue the process.  This will
      // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
      fseek( ifp, offset, SEEK_SET );
 
      // Read BLOCK_SIZE number of bytes from the input file and store them in our
      // data array. 
      int bytes  = fread( blocks[free_block_index], BLOCK_SIZE, 1, ifp );

      // If bytes == 0 and we haven't reached the end of the file then something is 
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
      if( bytes == 0 && !feof( ifp ) )
      {
        printf("An error occured reading from the input file.\n");
        return;
      }
      // Increment the index into the block array 
      if(copy_size > 0)
      {
        inodeindex ++;
        free_block_index = findFreeBlock();
      }

      // Clear the EOF file flag.
      clearerr( ifp );

      // Reduce copy_size by the BLOCK_SIZE bytes.
      copy_size -= BLOCK_SIZE;
      
      // Increase the offset into our input file by BLOCK_SIZE.  This will allow
      // the fseek at the top of the loop to position us to the correct spot.
      offset    += BLOCK_SIZE;

    }

    // We are done copying from the input file so close it out.
    fclose( ifp );
       
}
//changes the attribute of the file if the
//index of the file that exist is provided
//as the first parameter and
//changing the attribute to the one that is
//provided in the function call from the main
void attributeschange(int index, int attrib)
{
    inodeList[dir[index].inode].attributes = attrib;
}
//deletes the file from the file system if the name of
//file exist inside the filesystem
//the function will simply unlink the directory index and
//inode index from the file system
void deletefile(char *filename)
{
    int i,ret,attrib;
    ret = -1;
    for(i=0;i<128;i++)
    {
        if(strcmp(dir[i].name,filename)==0)
        {
            ret = i;
            break;
        }
    }
    //does not delete the file if it is marked as read only file
    if(ret == -1)
    {
        printf("File not Found.\n");
        return;
    }
    if(inodeList[dir[i].inode].attributes ==1)
    {
        printf("del: That file is marked read only.\n");
        return;
    }
    dir[i].valid = 0;
    inodeList[dir[i].inode].valid = 0;
}

void get(char *filename)
{
    
    int i,ret;
    ret = -1;
    for(i=0;i<128;i++)
    {
        if(strcmp(dir[i].name,filename)==0)
        {
            ret = i;
            break;
        }
    }
    if(ret == -1)
    {
        printf("File not Found.\n");
    }
    
    FILE *ofp;
    ofp = fopen(filename, "w");
    
    if( ofp == NULL )
    {
        printf("Could not open output file: %s\n", filename );
        perror("Opening output file returned");
        return;
    }
    
    
    // Initialize our offsets and pointers just we did above when reading from the file.
        int  block_index = 0;
        int  copy_size   ;
        int offset      = 0;
    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {
        
        int num_bytes;
        
        // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
        // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
        // end up with garbage at the end of the file.
        if( copy_size < BLOCK_SIZE )
        {
            num_bytes = copy_size;
        }
        else
        {
            num_bytes = BLOCK_SIZE;
        }
        
        // Write num_bytes number of bytes from our data array into our output file.
        fwrite( blocks[block_index], num_bytes, 1, ofp );
        
        // Reduce the amount of bytes remaining to copy, increase the offset into the file
        // and increment the block_index to move us to the next data block.
        copy_size -= BLOCK_SIZE;
        offset    += BLOCK_SIZE;
        block_index ++;
        
        // Since we've copied from the point pointed to by our current file pointer, increment
        // offset number of bytes so we will be ready to copy to the next area of our output file.
        fseek( ofp, offset, SEEK_SET );
    }
    
    // Close the output file, we're done.
    fclose( ofp );
}

int main()
{
    dir           = (struct Directory_Entry*)&blocks[0][0];
    freeInodeList = (uint8_t*)&blocks[7][0];
    freeBlockList = (uint8_t*)&blocks[8][0];
    inodeList     = (struct Inode*)&blocks[9];
    
    //int i;
    //for( i = 0; i<NUM_FILES; i++)
    //{
        //Inodes run from block 3 -131 so add 3 to the index 
    inodeList   = (struct Inode*)&blocks[3][0];
    //}
    
    
    
    initializeDirectory();
    initializeInodes();
    initializeInodeList();
    initializeBlockList();
    freeBlockList[7]=0;
    freeBlockList[8]=0;
    freeBlockList[9]=0;
   
        
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;


    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    
    if(strcmp(token[0],"put")==0)
    {
        put(token[1]);
    }
    else if(strcmp(token[0],"get")==0)
    {
        get(token[1]);
    }
    else if(strcmp(token[0],"del")==0)
    {
        deletefile(token[1]);
    }
    else if(strcmp(token[0],"list")==0)
    {
        //printf("list function goes here");
        list();
    }
    else if(strcmp(token[0],"df")==0)
    {
        printf("%d bytes free. \n",df());
    }
    else if(strcmp(token[0],"open")==0)
    {
        //printf("del function goes here");
        open(token[1]);
    }
    else if(strcmp(token[0],"close")==0)
    {
        //printf("del function goes here");
        closef(token[1]);
    }
    else if(strcmp(token[0],"createfs")==0)
    {
        
        //printf("creats goes here\n");
        createfs(token[1]);
             
    }
    else if(strcmp(token[0],"attrib")==0)
    {
        //printf("attribs goes here\n");
        int i,ret,attrib;
        ret = -1;
        for(i=0;i<128;i++)
        {
            if(strcmp(dir[i].name,token[2])==0)
            {
                ret = i;
                break;
            }
        }
        if(ret == -1)
        {
            printf("File not Found.\n");
        }
        else if(strcmp(token[1],"+r")==0)
        {
            attrib = 1;
            attributeschange(ret,attrib);
        }
        else if(strcmp(token[1],"-r")==0)
        {
            attrib = 0;
            attributeschange(ret,attrib);
        }
        else if(strcmp(token[1],"+h")==0)
        {
            attrib = -1;
            attributeschange(ret,attrib);
        }
        else if(strcmp(token[1],"-h")==0)
        {
            attrib = 0;
            attributeschange(ret,attrib);
        }
    }
    else if(strcmp(token[0],"exit")==0)
    {
        exit(0);
    }
      
    free( working_root );

  }
  return 0;
}
