#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <commctrl.h>

#define TB_SETIMAGELIST (WM_USER + 48)


HIMAGELIST g_hImageList = NULL;

#define CM_FILE_NEW	    9070
#define CM_FILE_OPEN	9071
#define CM_FILE_SAVE	9072
#define CM_FILE_EXIT	9073

#define CM_BLOCK_WIRE	9074
#define CM_BLOCK_NOT	9075
#define CM_BLOCK_DELTA	9076

#define CM_DIRECTION_UP	    9100
#define CM_DIRECTION_LEFT	9101
#define CM_DIRECTION_DOWN	9102
#define CM_DIRECTION_RIGHT	9103

#define ID_TOOLBAR      4998

HINSTANCE g_hInst;
HWND g_hToolBar;
HWND g_hMainWindow;


void draw_bitmap(HBITMAP image,int x,int y,byte image_single);
void draw_rectangle(int x1,int y1,int x2,int y2,COLORREF color1,COLORREF color2, int fill);
void drawstring2(int x, int y, const char *msg,COLORREF col,int w,int h);
HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent);


struct block;
void wire_draw(block *lnode);


bool runmain=1,running=0;

HBITMAP hbmBuffer =NULL;
HBITMAP hbmOldBuffer=NULL;
HDC hdcBuffer = NULL;
HDC hdcMain=NULL;

int window_width=640;
int window_height=480;
int view_left,view_top=0;

const int grid_w=10000;
const int grid_h=32;

HBITMAP block_sprite[32];
HBITMAP block_sprite_on[32];

int tile_size=32;

long fps,frame=0,maxfps;

int mousex,mousey=0;

const byte MouseMove=0;
const byte LeftDown=1;
const byte LeftUp=2;
const byte RightDown=3;
const byte RightUp=4;

const byte up=0;
const byte left=1;
const byte down=2;
const byte right=3;


const byte BWIRE=1;
const byte BNOT=2;
const byte BDELTA=3;

int block_types[32][4];
//void *block_draw[32];

struct block *head, *tail;

void append_node(struct dllist *lnode);
void insert_node(struct dllist *lnode, struct dllist *after);
void remove_node(struct dllist *lnode);

struct block *grid[grid_w+1][grid_h+1];

byte selectedblock=BWIRE;
byte selecteddirection=0;


void init_block_types()
{
  block_types[1][0]=BWIRE; // wire
  block_types[1][1]=0; //sprite index
  
  block_types[2][0]=BNOT; // not
  block_types[2][1]=1; //sprite index
  
  block_types[3][0]=BDELTA; // delta
  block_types[3][1]=2; //sprite index
  
  //ZeroMemory(grid, sizeof(grid));

}



int keyboard_key[5]; //key buffer

void add_key(int key)
{
  int i=0,found=0;
  for(i=0;i<5;i+=1)
  {
	if (keyboard_key[i] == key)
	{
	  found=1;
	  break;
	}
  }
  if (found==0)
  {
	for(i=0;i<5;i+=1)
	{
	  if (keyboard_key[i]==0)
	  {
	  keyboard_key[i]=key;
	  break;
	  }
	}
  }
}

void remove_key(int key)
{
  int i=0;
  for(i=0;i<5;i+=1)
  {
	if (keyboard_key[i] == key)
	{
	  keyboard_key[i]=0;
	}
  }
}

int check_key(int key)
{
  int i=0;
  for(i=0;i<5;i+=1)
  {
	if (keyboard_key[i] == key)
	{
	  return 1;
	}
  }
  return 0;
}






struct dllist {
 int number;
 struct dllist *next;
 struct dllist *prev;
};



struct block
{
  int x; //position on screen
  int y;
  int xx; // position in the grid
  int yy;
  byte direction;
  byte outputs;
  int data; // value signel
  byte inputs;
  bool connection[3];
  bool instack;
  byte movedir;
  byte settings;



  bool input;

  int blocktype;
  
  byte sprite_index;
  
  void (*draw_routine)(block *lnode);
  void (*func0_routine)(block *lnode);
  void (*func1_routine)(block *lnode);
  void (*func2_routine)(block *lnode);

  
  block *next;
  block *prev; 
};

byte bit4_shiftleft(byte val)
{
  bool bit3, bit2, bit1, bit0;
  bit0=val & 1;
  bit1=(val & 2) >> 1;
  bit2=(val & 4) >> 2;
  bit3=(val & 8) >> 3;
  
  val=0;

  if (bit3==1)
  {
   val+=1;
  }
  if (bit0==1)
  {
   val+=2;
  }
  if (bit1==1)
  {
   val+=4;
  }
  if (bit2==1)
  {
   val+=8;
  }
  
  return val;
}

byte bit4_set(bool bit3,bool bit2,bool bit1,bool bit0)
{
  return (8*bit3)+(4*bit2)+(2*bit1)+(1*bit0);     
}

bool bit4_test(byte val, byte bit)
{ 
  
  if (bit==0)
  {
    return val & 1;
  }
  else if (bit==1)
  { 
    return (val & 2) >> 1;
  }
  else if (bit==2)
  {
    return (val & 4) >> 2;
  }
  else if (bit==3)
  {
    return (val & 8) >> 3;
  }
  
  
  //return 0;   
}

struct execstack
{
  block *data;
  execstack *next;     
};

struct execstack *exectail;
int execsize=0;

void execstack_push(block *lnode)
{
     
  
  struct execstack *snode;

  snode = (struct execstack *)malloc(sizeof(struct execstack));
  snode->data=lnode;

  if (exectail != NULL)
  {
    snode->next=exectail;      
  }
  
  execsize+=1;
  
  exectail=snode;
  
}


block* execstack_pop()
{
  struct block *data;
  struct execstack *temp;
  
  if (exectail!=NULL)
  {
    data=exectail->data;
    
    temp=exectail;
    exectail=exectail->next;
    free(temp);
    execsize-=1;
  }
  
  return data;       
}

void scr_exec_push(block *lnode,byte dir)
{
  
  if (lnode==NULL)
  {
    return;               
  }
  
  
  switch(dir)
  {
    case up:
      
      if (bit4_test(lnode->inputs,0))
      {
        execstack_push(lnode);
      }
      
      break;
    case left:
      if (bit4_test(lnode->inputs,1))
      {
        execstack_push(lnode);
      }
      break;
    case down:
      if (bit4_test(lnode->inputs,2))
      {
        execstack_push(lnode);
      }
      break;
    case right:
      if (bit4_test(lnode->inputs,3))
      
      {
        execstack_push(lnode);
      }
      break;
  };
  
}


void wire_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(0,1,0,0);
      break;
    case left:
      lnode->inputs=bit4_set(1,0,0,0);
      break;
    case down:
      lnode->inputs=bit4_set(0,0,0,1);
      break;
    case right:
      lnode->inputs=bit4_set(0,0,1,0);
      break;
  };
        
}

void wire_exec(block *lnode)
{
  switch(lnode->direction)
  {
    case up:
      scr_exec_push(grid[ lnode->xx][lnode->yy-1],2);
      break;
    case left:
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };
}

void wire_step(block *lnode)
{
     
  block *nnn;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    lnode->data=nnn->data;      
  }
  
  
  
  if (lnode->input==1)
  {
  
    switch(lnode->direction)
    {
    case up:
      lnode->outputs=bit4_set(0,0,0,1);
      break;
    case left:
      lnode->outputs=bit4_set(0,0,1,0);
      break;
    case down:
      lnode->outputs=bit4_set(0,1,0,0);
      break;
    case right:
      lnode->outputs=bit4_set(1,0,0,0);
      break;
    };
  }
  
  
  if (lnode->input==0)
  {
     lnode->outputs=0;                 
  }
  

}


void wire_draw(block *lnode)
{
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
    
  }
}












void not_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(0,1,0,0);
      break;
    case left:
      lnode->inputs=bit4_set(1,0,0,0);
      break;
    case down:
      lnode->inputs=bit4_set(0,0,0,1);
      break;
    case right:
      lnode->inputs=bit4_set(0,0,1,0);
      break;
  };
        
}

void not_exec(block *lnode)
{
  switch(lnode->direction)
  {
    case up:
      scr_exec_push(grid[ lnode->xx][lnode->yy-1],2);
      break;
    case left:
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };
}

void not_step(block *lnode)
{
     
  block *nnn;
  
  lnode->input=0;
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      scr_exec_push(grid[ lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    lnode->data=nnn->data;             
  }
  
  if (lnode->input==0)
  {
  
    switch(lnode->direction)
    {
    case up:
      lnode->outputs=bit4_set(0,0,0,1);
      break;
    case left:
      lnode->outputs=bit4_set(0,0,1,0);
      break;
    case down:
      lnode->outputs=bit4_set(0,1,0,0);
      break;
    case right:
      lnode->outputs=bit4_set(1,0,0,0);
      break;
    };
  }
  
  
  if (lnode->input==1)
  {
     lnode->outputs=0;                 
  }
  
}


void not_draw(block *lnode)
{
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
    
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);  
    
  }
}









void delta_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(0,1,0,0);
      break;
    case left:
      lnode->inputs=bit4_set(1,0,0,0);
      break;
    case down:
      lnode->inputs=bit4_set(0,0,0,1);
      break;
    case right:
      lnode->inputs=bit4_set(0,0,1,0);
      break;
  };
        
}



void delta_exec(block *lnode)
{
  int xx,yy;
  xx=lnode->xx;
  yy=lnode->yy;
  
  switch(lnode->direction)
  {
    case up:
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx+1][yy],1);
 
      break;
    case left:
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx][yy+1],0);
      
      break;
    case down:
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx][yy+1],0);
      scr_exec_push(grid[xx+1][yy],1);
 
      break;
    case right:
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx][yy+1],0);
      scr_exec_push(grid[xx+1][yy],1);
 
      break;
  };
}

void delta_step(block *lnode)
{
     
  block *nnn;
  
  lnode->input=0;
  
  int xx,yy;
  xx=lnode->xx;
  yy=lnode->yy;
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx+1][yy],1);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx][yy+1],0);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      scr_exec_push(grid[xx-1][yy],3);
      scr_exec_push(grid[xx][yy+1],0);
      scr_exec_push(grid[xx+1][yy],1);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      scr_exec_push(grid[xx][yy-1],2);
      scr_exec_push(grid[xx][yy+1],0);
      scr_exec_push(grid[xx+1][yy],1);
      break;
  };   
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction); 
    lnode->data=nnn->data;            
  }
  
  if (lnode->input==1)
  {
  
    switch(lnode->direction)
    {
    case up:
      lnode->outputs=bit4_set(1,0,1,1);
      break;
    case left:
      lnode->outputs=bit4_set(0,1,1,1);
      break;
    case down:
      lnode->outputs=bit4_set(1,1,1,0);
      break;
    case right:
      lnode->outputs=bit4_set(1,1,0,1);
      break;
    };
  }
  
  
  if (lnode->input==0)
  {
     lnode->outputs=0;                 
  }
  
}


void delta_draw(block *lnode)
{
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
    
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
    
  }
}




/*
int main(void) {
 struct dllist *lnode;
 int i = 0;


 for(i = 0; i <= 5; i++) {
  lnode = (struct dllist *)malloc(sizeof(struct dllist));
  lnode->number = i;
  append_node(lnode);
 }


 for(lnode = head; lnode != NULL; lnode = lnode->next) {
  printf("%d\n", lnode->number);
 }

 
 while(head != NULL)
  remove_node(head);

 return 0;
}
*/

void append_node(struct block *lnode) {
 if(head == NULL) {
  head = lnode;
  lnode->prev = NULL;
 } else {
  tail->next = lnode;
  lnode->prev = tail;
 }

 tail = lnode;
 lnode->next = NULL;
}


void insert_node(struct block *lnode, struct block *after) {
 lnode->next = after->next;
 lnode->prev = after;

 if(after->next != NULL)
  after->next->prev = lnode;
 else
  tail = lnode;

 after->next = lnode;
}

void remove_node(struct block *lnode) {
 if(lnode->prev == NULL)
  head = lnode->next;
 else
  lnode->prev->next = lnode->next;

 if(lnode->next == NULL)
  tail = lnode->prev;
 else
  lnode->next->prev = lnode->prev;
}



HBITMAP tb_new;
HBITMAP tb_new_mask;
HBITMAP tb_open;
HBITMAP tb_open_mask;
HBITMAP tb_save;
HBITMAP tb_save_mask;

HBITMAP tb_wire;
HBITMAP tb_wire_mask;
HBITMAP tb_not;
HBITMAP tb_not_mask;
HBITMAP tb_delta;
HBITMAP tb_delta_mask;

HBITMAP tb_up;
HBITMAP tb_up_mask;

HBITMAP tb_left;
HBITMAP tb_left_mask;

HBITMAP tb_down;
HBITMAP tb_down_mask;

HBITMAP tb_right;
HBITMAP tb_right_mask;

void block_sprite_load(HWND hwnd)
{
  byte id;
  id=0;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "wire.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "wire_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "wire Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
  
  id=1;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "not.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "not_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "not Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION); 
    
  id=2;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "delta.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "delta_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "not Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  tb_new=(HBITMAP)LoadImage(NULL, "tb_new.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_new_mask=CreateBitmapMask(tb_new, RGB(255,0,255));
  
  tb_open=(HBITMAP)LoadImage(NULL, "tb_open.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_open_mask=CreateBitmapMask(tb_open, RGB(255,0,255));
  
  tb_save=(HBITMAP)LoadImage(NULL, "tb_save.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_save_mask=CreateBitmapMask(tb_save, RGB(255,0,255));
    
  tb_wire=(HBITMAP)LoadImage(NULL, "tb_wire.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_wire_mask=CreateBitmapMask(tb_wire, RGB(255,0,255));
  
  tb_not=(HBITMAP)LoadImage(NULL, "tb_not.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_not_mask=CreateBitmapMask(tb_not, RGB(255,0,255));
  
  tb_delta=(HBITMAP)LoadImage(NULL, "tb_delta.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_delta_mask=CreateBitmapMask(tb_delta, RGB(255,0,255));
  
  
  tb_up=(HBITMAP)LoadImage(NULL, "tb_up.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_up_mask=CreateBitmapMask(tb_up, RGB(255,0,255));
  
  tb_left=(HBITMAP)LoadImage(NULL, "tb_left.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_left_mask=CreateBitmapMask(tb_left, RGB(255,0,255));
  
  tb_down=(HBITMAP)LoadImage(NULL, "tb_down.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_down_mask=CreateBitmapMask(tb_down, RGB(255,0,255));
  
  tb_right=(HBITMAP)LoadImage(NULL, "tb_right.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_right_mask=CreateBitmapMask(tb_right, RGB(255,0,255));
  
     
}

block *peek;
int peekx,peeky;

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
char szClassName[ ] = "WindowsApp";



void draw_window()
{
  char string[255];
    int xx,yy;
  if (fps>maxfps)
            maxfps=fps;
            
          //sprintf(string,"%i",execsize);
          
          sprintf(string,"%i",fps);
          //sprintf(string,"%i",maxfps);
          
          draw_rectangle(0,0,window_width,window_height,RGB(0,0,0),RGB(255,0,0),1);
          //draw_rectangle(0,0,140,30,RGB(0,0,0),RGB(255,0,0),1);
          
          
          /*
          for(yy=(int)floor(view_top/32);yy<(int)floor(view_top/32)+15;yy++)
          {
            for(xx=(int)floor(view_left/32);xx<(int)floor(view_left/32)+20;xx++)
            {
              //if ( (xx>=0)&&(xx<=grid_w) && (yy>=0)&&(yy<=grid_h) )
              if (grid[xx][yy]!=NULL)
              {
                //draw_bitmap(block_sprite[0],(xx*32)-view_left,(yy*32)-view_top,0);
                grid[xx][yy]->draw_routine(grid[xx][yy]);
              }
            }
          }
          */
          
          for(yy=(int)floor(view_top/32);yy<(int)floor(view_top/32)+480;yy++)
          {
            for(xx=(int)floor(view_left/32);xx<(int)floor(view_left/32)+640;xx++)
            {
              //if ( (xx>=0)&&(xx<=grid_w) && (yy>=0)&&(yy<=grid_h) )
              if (grid[xx][yy]!=NULL)
              {
                //draw_bitmap(block_sprite[0],(xx*32)-view_left,(yy*32)-view_top,0);
                //grid[xx][yy]->draw_routine(grid[xx][yy]);
                SetPixel(hdcMain,xx,yy,RGB(0,0,255));
              }
            }
          }
          
          
          
          drawstring2(0,0,string,RGB(0,255,255),8,16);
          //sprintf(string,"%i",execsize);
          sprintf(string,"%i",sizeof(grid));
          drawstring2(0,20,string,RGB(255,0,255),8,16);
          sprintf(string,"%i",sizeof(block));
          drawstring2(0,40,string,RGB(255,0,255),8,16);
          
          if (peek!=NULL)
          {
            //sprintf(string,"%i",sizeof(block));
            drawstring2(peek->x-view_left,peek->y-view_top,"X",RGB(255,0,255),8,16);             
          }
          
          BitBlt(hdcMain, 0, 30, window_width , window_height , hdcBuffer, 0, 0, SRCCOPY);     
}



int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */
    
    InitCommonControls();
    g_hInst = hThisInstance;

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           "Windows App",       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           window_width,                 /* The programs width */
           window_height,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);


    
    /* Run the message loop. It will run until GetMessage() returns 0 */
    block *now;
    
    block *list[10000];
    int i,too=0;
    
    while (runmain)
    {
          if (PeekMessage(&messages, NULL, 0, 0, PM_REMOVE))
          {
            /* Translate virtual-key messages into character messages */
            TranslateMessage(&messages);
            /* Send message to WindowProcedure */
            DispatchMessage(&messages);              
          }
          
          
          if (execsize>0)
          {
            too=execsize;
            for(i=0;i<too;i++)               
            {
              now=execstack_pop();
              if (now!=NULL)
              {
              list[i]=now;      
              }
            }
            
            for(i=0;i<too;i++)               
            {
              if (list[i]!=NULL)
              {
              list[i]->func2_routine(list[i]);      
              }
            }
          }
          
          /*
          if (execsize>0)
          {
            
            peek=now;
            peekx=now->x;
            peeky=now->y;
            now->func2_routine(now);          
          }
          */
          
          //SetTimer(hwnd,1000,0,NULL);
          //draw_window();
          
          
          
          frame+=1;
    }
    
    
    /* The program return-value is 0 - The value that PostQuitMessage() gave */

    
    return messages.wParam;
}



struct block* block_create(int xx,int yy,byte blocktype)
{
 struct block *lnode;

  lnode = (struct block *)malloc(sizeof(struct block));
  append_node(lnode);
  
  lnode->x=xx*32;
  lnode->y=yy*32;
  lnode->xx=xx;
  lnode->yy=yy;
  lnode->blocktype=blocktype;
  lnode->direction=selecteddirection;
  lnode->data=0;
  //lnode->image_single=0;
  lnode->sprite_index=block_types[blocktype][1];
  lnode->inputs=0;
  lnode->instack=0;
  lnode->movedir=0;
  lnode->connection[0]=0;
  lnode->connection[1]=0;
  lnode->connection[2]=0;
  lnode->connection[3]=0;
  
  lnode->outputs=0;
  lnode->settings=0;
  
  if (blocktype==BWIRE)
  {
    lnode->draw_routine=wire_draw;     
    lnode->func0_routine=wire_input;    
    lnode->func1_routine=wire_exec;   
    lnode->func2_routine=wire_step;         
  }
  if (blocktype==BNOT)
  {
    lnode->draw_routine=not_draw;   
    lnode->func0_routine=not_input;    
    lnode->func1_routine=not_exec;   
    lnode->func2_routine=not_step;                  
  }
  if (blocktype==BDELTA)
  {
    lnode->draw_routine=delta_draw;   
    lnode->func0_routine=delta_input;    
    lnode->func1_routine=delta_exec;   
    lnode->func2_routine=delta_step;                  
  }
  
  
  return lnode;     
}


void mouse_event(byte event)
{
  int xx,yy;
  xx=(int)floor((mousex+view_left)/32);
  yy=(int)floor((mousey+view_top)/32);
  
  if (event==LeftDown)
  {
     if (grid[xx][yy]==NULL)
     {
       grid[xx][yy]=block_create(xx,yy,selectedblock);
       grid[xx][yy]->func0_routine(grid[xx][yy]);
       grid[xx][yy]->func2_routine(grid[xx][yy]);          
     }
     else
     {
       grid[xx][yy]->func1_routine(grid[xx][yy]);
       
       grid[xx][yy]->direction+=1;
       if (grid[xx][yy]->direction==4)
       {
         grid[xx][yy]->direction=0;                               
       }
       grid[xx][yy]->outputs=bit4_shiftleft(grid[xx][yy]->outputs);
       grid[xx][yy]->inputs=bit4_shiftleft(grid[xx][yy]->inputs);
       
       grid[xx][yy]->func1_routine(grid[xx][yy]);
     }
  }
  
  if (event==RightDown)
  {
     if (grid[xx][yy]!=NULL)
     {
       remove_node(grid[xx][yy]);
       free(grid[xx][yy]);
       grid[xx][yy]=NULL;                
     }
  }
}

bool isodd( int integer )
{

  if ( integer % 2== 0 )
     return 1;
  else
     return 0;
}



void keyboard_event()
{
  block *now;
   int xx,yy;
   
  if (check_key(VK_SPACE)==1)
  {
   
    
    for(yy=1;yy<grid_h-1;yy++)
    {
      for(xx=1;xx<grid_w-1;xx++)
      {
        if ((yy==1)&&(xx==1))
        {
          //selecteddirection=3;
          //grid[xx][yy]=block_create(xx,yy,BNOT);
          //grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);  
          
          selecteddirection=3;
          grid[xx+1][yy+1]=block_create(xx+1,yy+1,BNOT);
          grid[xx+1][yy+1]->func0_routine(grid[xx+1][yy+1]);
          grid[xx+1][yy+1]->func2_routine(grid[xx+1][yy+1]); 
          
          selecteddirection=0;
          grid[xx][yy+1]=block_create(xx,yy+1,BDELTA);
          grid[xx][yy+1]->func0_routine(grid[xx][yy+1]);
          grid[xx][yy+1]->func2_routine(grid[xx][yy+1]); 
          
          selecteddirection=1;
          grid[1][(grid_h-3)]=block_create(1,(grid_h-3),BDELTA);
          grid[1][(grid_h-3)]->func0_routine(grid[1][(grid_h-3)]);
          grid[1][(grid_h-3)]->func2_routine(grid[1][(grid_h-3)]);    
          
          //selecteddirection=0;
          //grid[0][0]=block_create(0,0,BDELTA);
          //grid[0][0]->func0_routine(grid[0][0]);
          //grid[0][0]->func2_routine(grid[0][0]);     
        }
        
        if ((xx==1) && (yy!=1) && (yy!=2) && (yy!=grid_h-3))
        {
          selecteddirection=0;
          grid[xx][yy]=block_create(xx,yy,BWIRE);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);          
        }
        
        
        if ( ( isodd(yy)==1) && (yy!=1)&& (yy!=grid_h-2) && (xx>=3) && (xx<=grid_w-4) )
        {
          selecteddirection=3;
          grid[xx][yy]=block_create(xx,yy,BWIRE);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
          
        }
        
        if   (( isodd(yy)==0) &&(yy!=1) && (xx>=3) && (xx<=grid_w-4) )
        {
          
          selecteddirection=1;
          grid[xx][yy]=block_create(xx,yy,BWIRE);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
          
        }
        
        if ( ( isodd(yy)==1)  && (xx==grid_w-3) )
        {
          
          selecteddirection=3;
          grid[xx][yy]=block_create(xx,yy,BDELTA);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
        }
        
        if ( ( isodd(yy)==0)  && (xx==grid_w-3) )
        {
          selecteddirection=2;
          grid[xx][yy]=block_create(xx,yy,BDELTA);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
        }
        
        if ( ( isodd(yy)==0) && (yy!=1)  && (xx==2) )
        {
          selecteddirection=1;
          grid[xx][yy]=block_create(xx,yy,BDELTA);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
        }
        
        if ( ( isodd(yy)==1) && (yy!=2) && (yy!=1) && (xx==2) )
        {
          selecteddirection=2;
          grid[xx][yy]=block_create(xx,yy,BDELTA);
          grid[xx][yy]->func0_routine(grid[xx][yy]);
          //grid[xx][yy]->func2_routine(grid[xx][yy]);   
        }
        
        
        
          

      }
    }
                           
  }
  
  if (check_key(VK_TAB)==1)
  {
    /*
    if (execsize>0)
          {
            now=execstack_pop();
            peek=now;
            now->func2_routine(now);          
          }                     
    */
  }
  
  int i;
  if (check_key(VK_HOME)==1)
  {
    
    for(i =0;i<2000;i++)
    {
    
                     
    xx=4+(4*i);
    yy=4;

    
    selecteddirection=0;
    grid[xx][yy]=block_create(xx,yy,BDELTA);
    grid[xx][yy]->func0_routine(grid[xx][yy]);  
    
    selecteddirection=3;
    xx+=1;
    grid[xx][yy]=block_create(xx,yy,BDELTA);
    grid[xx][yy]->func0_routine(grid[xx][yy]); 
    
    
    selecteddirection=2;
    //xx+=1;
    yy+=1;
    grid[xx][yy]=block_create(xx,yy,BDELTA);
    grid[xx][yy]->func0_routine(grid[xx][yy]);  
    
    selecteddirection=2;
    //xx+=1;
    yy+=1;
    grid[xx][yy]=block_create(xx,yy,BDELTA);
    grid[xx][yy]->func0_routine(grid[xx][yy]);   
    
    selecteddirection=1;
    xx-=1;
    //yy+=1;
    grid[xx][yy]=block_create(xx,yy,BDELTA);
    grid[xx][yy]->func0_routine(grid[xx][yy]);  
    
    selecteddirection=0;
    //xx-=1;
    yy-=1;
    grid[xx][yy]=block_create(xx,yy,BNOT);
    grid[xx][yy]->func0_routine(grid[xx][yy]);
    grid[xx][yy]->func2_routine(grid[xx][yy]);  
    
    }          
  }
  
  
  if (check_key(VK_END)==1)
  {
    block *now;
    if (execsize>0)
    {
      now=execstack_pop();
      now->func2_routine(now);          
    }                       
  }
  
  
  if (check_key(VK_LEFT)==1)
  {
    view_left-=32;
    if (view_left<0)
    {
      view_left=0;                
    }
  }
  
  if (check_key(VK_RIGHT)==1)
  {
    view_left+=32;
    if ((view_left)>(tile_size*grid_w)-window_width)
    {
      view_left= (tile_size*grid_w)-window_width;                                              
    }
  }
  
  if (check_key(VK_UP)==1)
  {
    view_top-=32;
    if (view_top<0)
    {
      view_top=0;                
    }
  }
  
  if (check_key(VK_DOWN)==1)
  {
    view_top+=32;
    if ((view_top)>(tile_size*grid_h)-window_height)
    {
      view_top= (tile_size*grid_h)-window_height;                                              
    }
  }
  
}


/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char string[255];
    int xx,yy;
    
    TBADDBITMAP tbab;
    TBBUTTON tbb[14];
    RECT rectTool;
    
    switch (message)                  /* handle the messages */
    {
        case WM_CREATE:
             
          
          init_block_types();
          hdcMain = GetDC(hwnd);
		  hdcBuffer = CreateCompatibleDC(hdcMain);
		  hbmBuffer = CreateCompatibleBitmap(hdcMain, window_width, 480);
		  hbmOldBuffer =(HBITMAP) SelectObject(hdcBuffer, hbmBuffer);
		  block_sprite_load(hwnd);
		  
		  if(SetTimer(hwnd, 1000, 31, NULL) == 0)
		   MessageBox(hwnd, "Could not SetTimer()!", "Error", MB_OK | MB_ICONEXCLAMATION);
		  
		  if(SetTimer(hwnd, 1001, 1000, NULL) == 0)
		   MessageBox(hwnd, "Could not SetTimer()!", "Error", MB_OK | MB_ICONEXCLAMATION);
		   
		  g_hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
            hwnd, (HMENU)ID_TOOLBAR, g_hInst, NULL);
            
          // Create the imagelist.
          g_hImageList = ImageList_Create(
          16, 16,   // Dimensions of individual bitmaps.
          ILC_COLOR24 | ILC_MASK,   // Ensures transparent background.
          3, 0);
          
          // Set the image list.
          ImageList_Add(g_hImageList,tb_new,tb_new_mask);
          ImageList_Add(g_hImageList,tb_open,tb_open_mask);
          ImageList_Add(g_hImageList,tb_save,tb_save_mask);
          
          ImageList_Add(g_hImageList,tb_wire,tb_wire_mask);
          ImageList_Add(g_hImageList,tb_not,tb_not_mask);
          ImageList_Add(g_hImageList,tb_delta,tb_delta_mask);
          
          ImageList_Add(g_hImageList,tb_up,tb_up_mask);
          ImageList_Add(g_hImageList,tb_left,tb_left_mask);
          ImageList_Add(g_hImageList,tb_down,tb_down_mask);
          ImageList_Add(g_hImageList,tb_right,tb_right_mask);
          
          SendMessage(g_hToolBar, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)g_hImageList);

          // Send the TB_BUTTONSTRUCTSIZE message, which is required for
          // backward compatibility.
          SendMessage(g_hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

          tbab.hInst = HINST_COMMCTRL;
          tbab.nID = IDB_STD_SMALL_COLOR;
          SendMessage(g_hToolBar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

          ZeroMemory(tbb, sizeof(tbb));
          
          tbb[0].iBitmap = MAKELONG(0, 0);
          tbb[0].fsState = TBSTATE_ENABLED;
          tbb[0].fsStyle = TBSTYLE_BUTTON;
          tbb[0].idCommand = CM_FILE_NEW;

          tbb[1].iBitmap = MAKELONG(1, 0);
          tbb[1].fsState = TBSTATE_ENABLED;
          tbb[1].fsStyle = TBSTYLE_BUTTON;
          tbb[1].idCommand = CM_FILE_OPEN;

          tbb[2].iBitmap = MAKELONG(2, 0);
          tbb[2].fsState = TBSTATE_ENABLED;
          tbb[2].fsStyle = TBSTYLE_BUTTON;
          tbb[2].idCommand = CM_FILE_SAVE;

          tbb[3].fsStyle = TBSTYLE_SEP;
          
          tbb[4].iBitmap = MAKELONG(3, 0);
          tbb[4].fsState = TBSTATE_ENABLED;
          tbb[4].fsStyle = TBSTYLE_CHECK;
          tbb[4].idCommand = CM_BLOCK_WIRE;
          
          tbb[5].iBitmap = MAKELONG(4, 0);
          tbb[5].fsState = TBSTATE_ENABLED;
          tbb[5].fsStyle = TBSTYLE_CHECK;
          tbb[5].idCommand = CM_BLOCK_NOT;
          
          tbb[6].iBitmap = MAKELONG(5, 0);
          tbb[6].fsState = TBSTATE_ENABLED;
          tbb[6].fsStyle = TBSTYLE_CHECK;
          tbb[6].idCommand = CM_BLOCK_DELTA;
          
          tbb[7].fsStyle = TBSTYLE_SEP;
          
          tbb[8].iBitmap = MAKELONG(6, 0);
          tbb[8].fsState = TBSTATE_ENABLED;
          tbb[8].fsStyle = TBSTYLE_CHECK;
          tbb[8].idCommand = CM_DIRECTION_UP;
          
          
          tbb[9].iBitmap = MAKELONG(7, 0);
          tbb[9].fsState = TBSTATE_ENABLED;
          tbb[9].fsStyle = TBSTYLE_CHECK;
          tbb[9].idCommand = CM_DIRECTION_LEFT;
          
          
          tbb[10].iBitmap = MAKELONG(8, 0);
          tbb[10].fsState = TBSTATE_ENABLED;
          tbb[10].fsStyle = TBSTYLE_CHECK;
          tbb[10].idCommand = CM_DIRECTION_DOWN;
          
          
          tbb[11].iBitmap = MAKELONG(9, 0);
          tbb[11].fsState = TBSTATE_ENABLED;
          tbb[11].fsStyle = TBSTYLE_CHECK;
          tbb[11].idCommand = CM_DIRECTION_RIGHT;


          SendMessage(g_hToolBar, TB_ADDBUTTONS, 12, (LPARAM)&tbb);
          
          
          //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_FILE_SAVE, MAKELONG(1, 0));
          ShowWindow(g_hToolBar, TRUE);
         //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_EDIT_UNDO, MAKELONG(1, 0));
         //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_EDIT_CUT, MAKELONG(1, 0));
         //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_EDIT_COPY, MAKELONG(1, 0));
         //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_EDIT_PASTE, MAKELONG(1, 0));
         break;
		  
        case  WM_MOUSEMOVE:
              
          mousex = lParam & 0xffff;
          mousey = (lParam >> 16)-30;
          mouse_event(MouseMove);
          break;
          
        case WM_LBUTTONDOWN:

          mouse_event(LeftDown);
          break;
          
        case WM_RBUTTONDOWN:

          mouse_event(RightDown);
          break;

        case WM_KEYUP:
          remove_key(wParam);
		  break;
        case WM_KEYDOWN:
             
		  add_key(wParam);
		  keyboard_event();
		  break;
        case WM_SIZE:

         SendMessage(g_hToolBar, TB_AUTOSIZE, 0, 0);
         GetWindowRect(g_hToolBar, &rectTool);
		  
        case WM_TIMER:
             
          if (wParam==1000)
          {
             
          
          
          if (fps>maxfps)
            maxfps=fps;
            
          //sprintf(string,"%i",execsize);
          
          sprintf(string,"%i",fps);
          //sprintf(string,"%i",maxfps);
          
          draw_rectangle(0,0,window_width,window_height,RGB(0,0,0),RGB(255,0,0),1);
          //draw_rectangle(0,0,140,30,RGB(0,0,0),RGB(255,0,0),1);
          
          
          
          for(yy=(int)floor(view_top/32);yy<(int)floor(view_top/32)+15;yy++)
          {
            for(xx=(int)floor(view_left/32);xx<(int)floor(view_left/32)+20;xx++)
            {
              //if ( (xx>=0)&&(xx<=grid_w) && (yy>=0)&&(yy<=grid_h) )
              if (grid[xx][yy]!=NULL)
              {
                //draw_bitmap(block_sprite[0],(xx*32)-view_left,(yy*32)-view_top,0);
                grid[xx][yy]->draw_routine(grid[xx][yy]);
              }
            }
          }
          
          
          
          drawstring2(0,0,string,RGB(0,255,255),8,16);
          sprintf(string,"%i",execsize);
          //sprintf(string,"%i",sizeof(grid));
          drawstring2(0,20,string,RGB(255,0,255),8,16);
          sprintf(string,"%i",sizeof(block));
          drawstring2(0,40,string,RGB(255,0,255),8,16);
          
          if (peek!=NULL)
          {
            //sprintf(string,"%i",sizeof(block));
            drawstring2(peek->x-view_left,peek->y-view_top,"X",RGB(255,0,255),8,16);             
          }
          
          BitBlt(hdcMain, 0, 30, window_width , window_height , hdcBuffer, 0, 0, SRCCOPY);
          
          }
          
          if (wParam==1001)
          {
            fps=frame;  
            frame=0;               
          }
          
          break;
          
          
       case WM_COMMAND:
      
         switch(LOWORD(wParam))
         {
           case CM_FILE_NEW:
                
             break;
             
           case CM_BLOCK_WIRE:
                
             selectedblock=BWIRE;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_NOT:
                
             selectedblock=BNOT;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_DELTA:
                
             selectedblock=BDELTA;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(1, 0) );
             break;
             
           case CM_DIRECTION_UP:
             selecteddirection=up;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(1, 0) ); 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_LEFT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_DOWN ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_RIGHT ,  MAKELONG(0, 0) );
             break;
             
           case CM_DIRECTION_LEFT:
             selecteddirection=left;   
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(0, 0) ); 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_LEFT ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_DOWN ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_RIGHT ,  MAKELONG(0, 0) );
             break;
             
           case CM_DIRECTION_DOWN:
             selecteddirection=down;  
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(0, 0) ); 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_LEFT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_DOWN ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_RIGHT ,  MAKELONG(0, 0) ); 
             break;
             
           case CM_DIRECTION_RIGHT:
             selecteddirection=right; 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(0, 0) ); 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_LEFT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_DOWN ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_RIGHT ,  MAKELONG(1, 0) );  
             break;
           
           
         };
        
		  break;
        case WM_DESTROY:
            
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            runmain=0;
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

void draw_bitmap(HBITMAP image,int x,int y,byte image_single)
{
  BITMAP bm;
  HDC hdcMem = CreateCompatibleDC(hdcMain);
  HBITMAP hbmOld =(HBITMAP) SelectObject(hdcMem, image);
  GetObject(image, sizeof(bm), &bm);
  BitBlt(hdcBuffer, x, y, tile_size, tile_size, hdcMem,  (tile_size*image_single) ,0, SRCCOPY);
  SelectObject(hdcMem, hbmOld);
  DeleteDC(hdcMem);
}

void draw_rectangle(int x1,int y1,int x2,int y2,COLORREF color1,COLORREF color2, int fill)
{
  HPEN hpen=CreatePen(0,1,color1 );
  SelectObject(hdcBuffer,hpen);
  HBRUSH hbr=CreateSolidBrush( color2 );
  if (fill==1)
  {
  SelectObject(hdcBuffer,hbr);
  }
  else
  {
  SelectObject(hdcBuffer,(HBRUSH)GetStockObject(HOLLOW_BRUSH));
  }
  Rectangle(hdcBuffer,x1,y1,x2,y2);
  DeleteObject(hpen);
  DeleteObject(hbr);
}

void drawstring2(int x, int y, const char *msg,COLORREF col,int w,int h)
{
   HFONT hfnt, hOldFont;
   int result=0;
   //hfnt =(HFONT) GetStockObject(ANSI_VAR_FONT);
   HFONT myfont;
   myfont=CreateFont(h,w,0,0,0,0,0,0,DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
   DEFAULT_QUALITY,(DEFAULT_PITCH|FF_DONTCARE),"Arial");
   result=SetBkMode(hdcBuffer,1); // 1 or 2
   col=SetTextColor(hdcBuffer,col);
	if (hOldFont =(HFONT) SelectObject(hdcBuffer, myfont)) {
		TextOut(hdcBuffer, x, y, msg, strlen(msg));
		SelectObject(hdcMain, hOldFont);
	}
	//DeleteObject(hfnt);
	DeleteObject(hOldFont);
	DeleteObject(myfont);
	result=SetBkMode(hdcBuffer,0); // 1 or 2
	col=SetTextColor(hdcBuffer,RGB(0,0,0));
}

HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent)
{
	HDC hdcMem, hdcMem2;
	HBITMAP hbmMask;
	BITMAP bm;
	GetObject(hbmColour, sizeof(BITMAP), &bm);
	hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
	hdcMem = CreateCompatibleDC(0);
	hdcMem2 = CreateCompatibleDC(0);
	SelectObject(hdcMem, hbmColour);
	SelectObject(hdcMem2, hbmMask);
	SetBkColor(hdcMem, crTransparent);
	BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
	BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem2, 0, 0, SRCINVERT);
	DeleteDC(hdcMem);
	DeleteDC(hdcMem2);
	return hbmMask;
}
