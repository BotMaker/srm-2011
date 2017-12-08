#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <commctrl.h>


#define TB_SETIMAGELIST (WM_USER + 48)
#define ID_TOOLBAR      4998

#define CM_FILE_NEW	    9070
#define CM_FILE_OPEN	9071
#define CM_FILE_SAVE	9072
#define CM_FILE_EXIT	9073

#define CM_BLOCK_WIRE	9074
#define CM_BLOCK_NOT	9075
#define CM_BLOCK_DELTA	9076
#define CM_BLOCK_PUSH	9077
#define CM_BLOCK_NOR	9078
#define CM_BLOCK_AND	9079
#define CM_BLOCK_OR	    9080
#define CM_BLOCK_TOGGLE	9081
#define CM_BLOCK_PULSER	9082

#define CM_BLOCK_RIGHT_SLIDER	9083
#define CM_BLOCK_LEFT_SLIDER	9084


#define CM_DIRECTION_UP	    9100
#define CM_DIRECTION_LEFT	9101
#define CM_DIRECTION_DOWN	9102
#define CM_DIRECTION_RIGHT	9103

#define CM_RUN	    9150
#define CM_RUNSTEP	9151
#define CM_RUNSTOP  9152

#define CM_CONNECT    9200
#define CM_DISCONNECT 9201
#define CM_SELECT     9202



HIMAGELIST g_hImageList = NULL;

HINSTANCE g_hInst;
HWND g_hToolBar;
HWND MainWindow;


void draw_bitmap(HBITMAP image,int x,int y,byte image_single);
void draw_rectangle(int x1,int y1,int x2,int y2,COLORREF color1,COLORREF color2, int fill);
void drawstring2(int x, int y, const char *msg,COLORREF col,int w,int h);
HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent);
void draw_line(int x1,int y1,int x2,int y2,COLORREF color1);
void draw_int(int x, int y, int num,COLORREF col);


struct block;
void wire_draw(block *lnode);

int block_id=0;

bool runmain=1,running=0;

HBITMAP hbmBuffer =NULL;
HBITMAP hbmOldBuffer=NULL;
HDC hdcBuffer = NULL;
HDC hdcMain=NULL;

int window_width=800;
int window_height=600;
int view_left,view_top=0;

const int grid_w=1000;
const int grid_h=1000;

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
const byte BPUSH=4;
const byte BNOR=5;
const byte BAND=6;
const byte BOR=7;
const byte BTOGGLE=8;
const byte BPULSER=9;
const byte BRIGHT_SLIDER=10;
const byte BLEFT_SLIDER=11;

int block_types[32][4];
//void *block_draw[32];

struct block *head, *tail;

void append_node(struct dllist *lnode);
void insert_node(struct dllist *lnode, struct dllist *after);
void remove_node(struct dllist *lnode);

struct block *grid[grid_w+1][grid_h+1];

byte selectedblock=BWIRE;
byte selecteddirection=0;
byte selectedmode=0;

int x_lookup[4],y_lookup[4],xx_lookup[4],yy_lookup[4];

int scr_opposite(int dir)
{
  return (dir+2) % 4;
}

void init_block_types()
{
  block_types[1][0]=BWIRE; // wire
  block_types[1][1]=0; //sprite index
  
  block_types[2][0]=BNOT; // not
  block_types[2][1]=1; //sprite index
  
  block_types[3][0]=BDELTA; // delta
  block_types[3][1]=2; //sprite index
  
  block_types[4][0]=BPUSH; // delta
  block_types[4][1]=3; //sprite index
  
  block_types[5][0]=BNOR; // nor
  block_types[5][1]=4; //sprite index
  
  block_types[6][0]=BAND; // and
  block_types[6][1]=5; //sprite index
  
  block_types[7][0]=BOR; // or
  block_types[7][1]=6; //sprite index
  
  block_types[8][0]=BTOGGLE; // toggle
  block_types[8][1]=7; //sprite index
  
  block_types[9][0]=BPULSER; // pulser
  block_types[9][1]=8; //sprite index
  
  block_types[10][0]=BRIGHT_SLIDER; // right slider
  block_types[10][1]=9; //sprite index
  
  block_types[11][0]=BLEFT_SLIDER; // right slider
  block_types[11][1]=10; //sprite index
  
  //ZeroMemory(grid, sizeof(grid));
  
  x_lookup[0]=0;
  x_lookup[1]=-1;
  x_lookup[2]=0;
  x_lookup[3]=1;

  y_lookup[0]=-1;
  y_lookup[1]=0;
  y_lookup[2]=1;
  y_lookup[3]=0;

  xx_lookup[0]=0;
  xx_lookup[1]=-32;
  xx_lookup[2]=0;
  xx_lookup[3]=32;

  yy_lookup[0]=-32;
  yy_lookup[1]=0;
  yy_lookup[2]=32;
  yy_lookup[3]=0;

}

int scr_xy_to_dir(int xx,int yy)
{

  if ((xx==0) && (yy==-1))
  {
    return 2;
  }
  else if ((xx==0) && (yy==1))
  {
    return 0;
  }
  else if ((xx==-1) && (yy==0))
  {
    return 3;
  }
  else if ((xx==1) && (yy==0))
  {
    return 1;
  }
  else
  {
    return -1;
  }

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
  byte connection[4];
  bool instack;
  byte movedir;
  byte settings;

  int id;

  bool input;

  int blocktype;
  
  byte sprite_index;
  
  byte old_state;
  byte toggle;
  
  void (*draw_routine)(block *lnode);
  void (*func0_routine)(block *lnode);
  void (*func1_routine)(block *lnode);
  void (*func2_routine)(block *lnode);
  
  //void (*funcmove0_routine)(block *lnode);
  //void (*funcmove1_routine)(block *lnode);

  
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
  //return ( val & (1<<bit) ) >> bit;  
}


struct movelist
{
  block *data;
  movelist *next;     
};

struct movelist *movehead, *movetail;
int movesize=0;


void movelist_push(block *lnode)
{
     
  struct movelist *snode;

  snode = (struct movelist *)malloc(sizeof(struct movelist));
  snode->data=lnode;

  if (movesize==0)
  {
    movehead=snode;
    movetail=snode;           
  }
  else
  {
    movetail->next=snode;
    movetail=snode;
  }

  movesize+=1;
  
}

block* movelist_pop()
{
  struct block *data;
  struct movelist *temp;
  
  if (movehead!=NULL)
  {
    data=movehead->data;
    temp=movehead;
    movehead=movehead->next;
    free(temp);
    movesize-=1;
  }
  
  return data;       
}

struct openlist
{
  block *data;
  openlist *next;     
};

struct openlist *openhead, *opentail;
int opensize=0;


void openlist_push(block *lnode)
{
     
  struct openlist *snode;

  snode = (struct openlist *)malloc(sizeof(struct openlist));
  snode->data=lnode;

  if (opensize==0)
  {
    openhead=snode;
    openhead->data=lnode;
    opentail=snode;           
  }
  else
  {
    opentail->next=snode;
    opentail=snode;
  }

  opensize+=1;
  
}

block* openlist_pop()
{
  struct block *data;
  struct openlist *temp;
  
  if (openhead!=NULL)//&& (openhead->data!=NULL)
  {
    data=openhead->data;
    temp=openhead;
    openhead=openhead->next;
    free(temp);
    opensize-=1;
  }
  
  return data;       
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
      //lnode->outputs=bit4_set(0,0,0,1);
      lnode->outputs=1;
      break;
    case left:
      //lnode->outputs=bit4_set(0,0,1,0);
      lnode->outputs=2;
      break;
    case down:
      //lnode->outputs=bit4_set(0,1,0,0);
      lnode->outputs=4;
      break;
    case right:
      //lnode->outputs=bit4_set(1,0,0,0);
      lnode->outputs=8;
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
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
  
  //draw_int(x,y,x,RGB(128,0,128));
  //draw_int(x,y,lnode->id,RGB(128,0,128));
  //draw_int(x,y+18,lnode->connection[4],RGB(128,0,128));
}



void scr_connections_move(block *lnode)
{
  int i,tx,ty=0;
  block *nnn;
  
  
  for(i=0;i<4;i++)
  {
              
    if (lnode->connection[i]==1)
    {

      tx = lnode->xx + x_lookup[i];
      ty = lnode->yy + y_lookup[i];
      nnn=grid[tx][ty];
    
      if (nnn!=NULL)
      {
        
        if (nnn->instack==0)
        {
          nnn->movedir=lnode->movedir;
          nnn->instack=1;
          openlist_push(nnn);
        }
      }
    }
    
  }
  
}




void block_move0(block *lnode)
{
  int nnnx,nnny=0;
  block *nnn;
  
  grid[lnode->xx][lnode->yy]=NULL;

  nnnx =lnode->xx + x_lookup[lnode->movedir];
  nnny =lnode->yy + y_lookup[lnode->movedir];


  nnn = grid[nnnx][nnny];

  if (nnn==NULL)
  {
    scr_connections_move(lnode);
  }

  if (nnn!=NULL)
  {
  
    if (nnn->instack==0)
    {
      scr_connections_move(lnode);
      
      if (lnode->connection[lnode->movedir]==0)
      {
        nnn->movedir=lnode->movedir;
        nnn->instack=1;
        openlist_push(nnn);
      }
      
    }
  } 
}


void block_move1(block *lnode)
{
  lnode->xx += x_lookup[lnode->movedir];
  lnode->yy += y_lookup[lnode->movedir];
  grid[lnode->xx][lnode->yy]=lnode;
  lnode->x += xx_lookup[lnode->movedir];
  lnode->y += yy_lookup[lnode->movedir];
  lnode->instack=0;   
}


void blocks_move()
{
  block *nnn;
  while (opensize>0)
  {
    nnn=openlist_pop();
    block_move0(nnn);
    movelist_push(nnn);
  }

  
  while (movesize>0)
  {
    nnn=movelist_pop();
    block_move1(nnn);
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
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
    
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);  
    
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
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
      //lnode->outputs=bit4_set(1,0,1,1);
      lnode->outputs=11;
      break;
    case left:
      //lnode->outputs=bit4_set(0,1,1,1);
      lnode->outputs=7;
      break;
    case down:
      //lnode->outputs=bit4_set(1,1,1,0);
      lnode->outputs=14;
      break;
    case right:
      //lnode->outputs=bit4_set(1,1,0,1);
      lnode->outputs=13;
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
     
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
    
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
    
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
}






void push_input(block *lnode)
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

void push_exec(block *lnode)
{
  /*
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
  */
}

void push_step(block *lnode)
{
     
  block *nnn;
  int xx,yy;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    if (lnode->input==1)
    {
      xx=lnode->xx + x_lookup[lnode->direction];
      yy=lnode->yy + y_lookup[lnode->direction];
      nnn=grid[xx][yy];
      if (nnn!=NULL)
      {
        if (nnn->instack==0)
        {
        openlist_push(nnn);
        nnn->movedir=lnode->direction;
        nnn->instack=1;                     
        }
      }
    }
  }  

}


void push_draw(block *lnode)
{
  draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
}


void right_slider_input(block *lnode)
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

void right_slider_exec(block *lnode)
{
  /*
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
  */
}

void right_slider_step(block *lnode)
{
     
  block *nnn;
  int xx,yy;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    if (lnode->input==1)
    {
      xx=lnode->xx + x_lookup[lnode->direction];
      yy=lnode->yy + y_lookup[lnode->direction];
      nnn=grid[xx][yy];
      if (nnn!=NULL)
      {
        if (nnn->instack==0)
        {
        openlist_push(nnn);
        nnn->movedir=(lnode->direction+3)%4;
        nnn->instack=1;                     
        }
      }
    }
  }  

}


void right_slider_draw(block *lnode)
{
  draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
}




void left_slider_input(block *lnode)
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

void left_slider_exec(block *lnode)
{
  /*
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
  */
}

void left_slider_step(block *lnode)
{
     
  block *nnn;
  int xx,yy;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    if (lnode->input==1)
    {
      xx=lnode->xx + x_lookup[lnode->direction];
      yy=lnode->yy + y_lookup[lnode->direction];
      nnn=grid[xx][yy];
      if (nnn!=NULL)
      {
        if (nnn->instack==0)
        {
        openlist_push(nnn);
        nnn->movedir=(lnode->direction+5)% 4;
        nnn->instack=1;                     
        }
      }
    }
  }  

}


void left_slider_draw(block *lnode)
{
  draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
}




void nor_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case left:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
    case down:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case right:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
  };
        
}

void nor_exec(block *lnode)
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

void nor_step(block *lnode)
{
     
  block *nnn1,*nnn2;
  int input1=0,input2=0;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn1=grid[lnode->xx-1][lnode->yy];
      nnn2=grid[lnode->xx+1][lnode->yy];
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn1=grid[lnode->xx][lnode->yy+1];
      nnn2=grid[lnode->xx][lnode->yy-1];
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn1=grid[lnode->xx+1][lnode->yy];
      nnn2=grid[lnode->xx-1][lnode->yy]; 
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn1=grid[lnode->xx][lnode->yy-1];
      nnn2=grid[lnode->xx][lnode->yy+1]; 
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn1!=NULL)
  {
    
    /*
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    lnode->data=nnn->data;      
    */
    
    switch(lnode->direction)
    {
    case up:
      input1=bit4_test(nnn1->outputs,3);
      break;
    case left:
      input1=bit4_test(nnn1->outputs,0);
      break;
    case down:
      input1=bit4_test(nnn1->outputs,1);
      break;
    case right:
      input1=bit4_test(nnn1->outputs,2);
      break;
    };
  
  }
  
  if (nnn2!=NULL)
  {
    
    switch(lnode->direction)
    {
    case up:
      input2=bit4_test(nnn2->outputs,1);
      break;
    case left:
      input2=bit4_test(nnn2->outputs,2);
      break;
    case down:
      input2=bit4_test(nnn2->outputs,3);
      break;
    case right:
      input2=bit4_test(nnn2->outputs,0);
      break;
    };
  
  }
  
  
  if ((input1==0) && (input2==0))
  {
    if (lnode->outputs==0)
    {
      nor_exec(lnode);                    
    }
    
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
  
  
  if ((input1==1) or (input2==1))
  {
     if (lnode->outputs!=0)
     {
       nor_exec(lnode);                     
     }
     lnode->outputs=0;                
  }
  

}


void nor_draw(block *lnode)
{
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
  

}






void and_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case left:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
    case down:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case right:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
  };
        
}

void and_exec(block *lnode)
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

void and_step(block *lnode)
{
     
  block *nnn1,*nnn2;
  int input1=0,input2=0;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn1=grid[lnode->xx-1][lnode->yy];
      nnn2=grid[lnode->xx+1][lnode->yy];
      
      break;
    case left:
      nnn1=grid[lnode->xx][lnode->yy+1];
      nnn2=grid[lnode->xx][lnode->yy-1];
      
      break;
    case down:
      nnn1=grid[lnode->xx+1][lnode->yy];
      nnn2=grid[lnode->xx-1][lnode->yy]; 
      
      break;
    case right:
      nnn1=grid[lnode->xx][lnode->yy-1];
      nnn2=grid[lnode->xx][lnode->yy+1]; 
      
      break;
  };   
  
  
  if (nnn1!=NULL)
  {
        
    switch(lnode->direction)
    {
    case up:
      input1=bit4_test(nnn1->outputs,3);
      break;
    case left:
      input1=bit4_test(nnn1->outputs,0);
      break;
    case down:
      input1=bit4_test(nnn1->outputs,1);
      break;
    case right:
      input1=bit4_test(nnn1->outputs,2);
      break;
    };
  
  }
  
  if (nnn2!=NULL)
  {
    
    switch(lnode->direction)
    {
    case up:
      input2=bit4_test(nnn2->outputs,1);
      break;
    case left:
      input2=bit4_test(nnn2->outputs,2);
      break;
    case down:
      input2=bit4_test(nnn2->outputs,3);
      break;
    case right:
      input2=bit4_test(nnn2->outputs,0);
      break;
    };
  
  }
  
  
  if ((input1==1) && (input2==1))
  {
    if (lnode->outputs==0)
    {
      and_exec(lnode);                    
    }
    
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
  
  
  if ((input1==0) or (input2==0))
  {
     if (lnode->outputs!=0)
     {
       and_exec(lnode);                     
     }
     lnode->outputs=0;                 
  }
  

}


void and_draw(block *lnode)
{
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
  

}




void or_input(block *lnode)
{
  lnode->inputs=0;
  
  switch(lnode->direction)
  {
    case up:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case left:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
    case down:
      lnode->inputs=bit4_set(1,0,1,0);
      break;
    case right:
      lnode->inputs=bit4_set(0,1,0,1);
      break;
  };
        
}

void or_exec(block *lnode)
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

void or_step(block *lnode)
{
     
  block *nnn1,*nnn2;
  int input1=0,input2=0;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn1=grid[lnode->xx-1][lnode->yy];
      nnn2=grid[lnode->xx+1][lnode->yy];
      
      break;
    case left:
      nnn1=grid[lnode->xx][lnode->yy+1];
      nnn2=grid[lnode->xx][lnode->yy-1];
      
      break;
    case down:
      nnn1=grid[lnode->xx+1][lnode->yy];
      nnn2=grid[lnode->xx-1][lnode->yy]; 
      
      break;
    case right:
      nnn1=grid[lnode->xx][lnode->yy-1];
      nnn2=grid[lnode->xx][lnode->yy+1]; 
      
      break;
  };   
  
  
  if (nnn1!=NULL)
  {
    
    
    switch(lnode->direction)
    {
    case up:
      input1=bit4_test(nnn1->outputs,3);
      break;
    case left:
      input1=bit4_test(nnn1->outputs,0);
      break;
    case down:
      input1=bit4_test(nnn1->outputs,1);
      break;
    case right:
      input1=bit4_test(nnn1->outputs,2);
      break;
    };
  
  }
  
  if (nnn2!=NULL)
  {
    
    switch(lnode->direction)
    {
    case up:
      input2=bit4_test(nnn2->outputs,1);
      break;
    case left:
      input2=bit4_test(nnn2->outputs,2);
      break;
    case down:
      input2=bit4_test(nnn2->outputs,3);
      break;
    case right:
      input2=bit4_test(nnn2->outputs,0);
      break;
    };
  
  }
  
  
  if ((input1==1) || (input2==1))
  {
   
    
    if (lnode->outputs==0)
    {
      and_exec(lnode);                    
    }
    
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
  
  
  if ((input1==0) && (input2==0))
  {
     if (lnode->outputs!=0)
     {
       and_exec(lnode);                     
     }
     lnode->outputs=0;                 
  }
  

}


void or_draw(block *lnode)
{
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
  

}




void toggle_input(block *lnode)
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

void toggle_exec(block *lnode)
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

void toggle_step(block *lnode)
{
     
  block *nnn;
  bool bit;
  
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
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
      bit=bit4_test(lnode->outputs,0);
      lnode->outputs=bit4_set(0,0,0,not(bit));
      scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      bit=bit4_test(lnode->outputs,1);
      lnode->outputs=bit4_set(0,0,not(bit),0);
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      bit=bit4_test(lnode->outputs,2);
      lnode->outputs=bit4_set(0,not(bit),0,0);
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      bit=bit4_test(lnode->outputs,3);
      lnode->outputs=bit4_set(not(bit),0,0,0);
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
    };
  }
   
}


void toggle_draw(block *lnode)
{
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }
  
  //draw_int(x,y,x,RGB(128,0,128));
  //draw_int(x,y,lnode->id,RGB(128,0,128));
  //draw_int(x,y+18,lnode->connection[4],RGB(128,0,128));
}



void pulser_input(block *lnode)
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

void pulser_exec(block *lnode)
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

void pulser_step(block *lnode)
{
     
  block *nnn,*nnn2;
  
  switch(lnode->direction)
  {
    case up:
      nnn=grid[lnode->xx][lnode->yy+1];
      
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      nnn=grid[lnode->xx+1][lnode->yy];
      
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      nnn=grid[lnode->xx][lnode->yy-1];
       
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      nnn=grid[lnode->xx-1][lnode->yy];
       
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);
    lnode->data=nnn->data;      
  }
  
  if ((lnode->input==1) && (lnode->toggle==2))
  {
    lnode->outputs=0;
    lnode->toggle=3; 
    switch(lnode->direction)
    {
    case up:
      
      //execstack_push(lnode);
      scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      
      //execstack_push(lnode);
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      
      //execstack_push(lnode);
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      
      //execstack_push(lnode); 
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
    };                    
  }
  
  if ((lnode->input==1) && (lnode->toggle==1))
  {
    lnode->outputs=0;
    lnode->toggle=2;
    
    switch(lnode->direction)
    {
    case up:
      
      execstack_push(lnode);
      //scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      break;
    case left:
      
      execstack_push(lnode);
      //scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      break;
    case down:
      
      execstack_push(lnode);
      //scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      break;
    case right:
      
      execstack_push(lnode); 
      //scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
    };
                
  }
  
  if ((lnode->input==0))
  {
    lnode->outputs=0;  
    lnode->toggle=0;                    
  }


  if ((lnode->input==1) && (lnode->toggle==0))
  {
    
    lnode->toggle=1;
    
    
    //execstack_push(lnode);
    //execnextstack_push(lnode,1);
    
    
    switch(lnode->direction)
    {
    case up:
      
      lnode->outputs=bit4_set(0,0,0,1);
      //nnn2=grid[lnode->xx][lnode->yy-1];
      execstack_push(lnode);
      scr_exec_push( grid[lnode->xx][lnode->yy-1],2);
      
      break;
    case left:
      
      lnode->outputs=bit4_set(0,0,1,0);
      //nnn2=grid[ lnode->xx-1][lnode->yy];
      execstack_push(lnode);
      scr_exec_push(grid[ lnode->xx-1][lnode->yy],3);
      
      break;
    case down:
      
      lnode->outputs=bit4_set(0,1,0,0);
      //nnn2=grid[ lnode->xx][lnode->yy+1];
      execstack_push(lnode);
      scr_exec_push(grid[ lnode->xx][lnode->yy+1],0);
      
      break;
    case right:
      
      lnode->outputs=bit4_set(1,0,0,0);
      //nnn2=grid[ lnode->xx+1][lnode->yy];
      execstack_push(lnode);
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      
      break;
    };
    
    if (nnn2!=NULL)
    {
    //nnn2->func2_routine(nnn2);
    }
    
  }
   
}


void pulser_draw(block *lnode)
{
  int x,y;
  x=lnode->x-view_left;
  y=lnode->y-view_top;
  
  if (lnode->outputs==0)
  {
    draw_bitmap(block_sprite[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction);
  }
  else
  {
    draw_bitmap(block_sprite_on[lnode->sprite_index],lnode->x-view_left,lnode->y-view_top,lnode->direction); 
  }
  
  if (lnode->connection[0]==1)
  {
    draw_line(x,y+3,x+32,y+3,RGB(0,0,255));
  }

  if (lnode->connection[1]==1)
  {
    draw_line(x+3,y,x+3,y+32,RGB(0,0,255));
  }

  if (lnode->connection[2]==1)
  {
    draw_line(x,y+29,x+32,y+29,RGB(0,0,255));
  }

  if (lnode->connection[3]==1)
  {
    draw_line(x+29,y,x+29,y+32,RGB(0,0,255));
  }

}







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

HBITMAP tb_push;
HBITMAP tb_push_mask;

HBITMAP tb_up;
HBITMAP tb_up_mask;
HBITMAP tb_left;
HBITMAP tb_left_mask;
HBITMAP tb_down;
HBITMAP tb_down_mask;
HBITMAP tb_right;
HBITMAP tb_right_mask;

HBITMAP tb_run;
HBITMAP tb_run_mask;
HBITMAP tb_runstop;
HBITMAP tb_runstop_mask;
HBITMAP tb_runstep;
HBITMAP tb_runstep_mask;

HBITMAP tb_connect;
HBITMAP tb_connect_mask;
HBITMAP tb_disconnect;
HBITMAP tb_disconnect_mask;

HBITMAP tb_select;
HBITMAP tb_select_mask;

HBITMAP tb_nor;
HBITMAP tb_nor_mask;

HBITMAP tb_and;
HBITMAP tb_and_mask;

HBITMAP tb_or;
HBITMAP tb_or_mask;

HBITMAP tb_toggle;
HBITMAP tb_toggle_mask;

HBITMAP tb_pulser;
HBITMAP tb_pulser_mask;

HBITMAP tb_right_slider;
HBITMAP tb_right_slider_mask;

HBITMAP tb_left_slider;
HBITMAP tb_left_slider_mask;

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
    MessageBox(hwnd, "delta Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=3;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "push.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if (block_sprite[id] == NULL)
    MessageBox(hwnd, "push Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=4;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "nor.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "nor_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "nor Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=5;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "and.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "and_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "and Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=6;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "or.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "or_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "or Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=7;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "toggle.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "toggle_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "toggle Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=8;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "pulser.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[id]=(HBITMAP)LoadImage(NULL, "pulser_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[id] == NULL) || (block_sprite_on[id] == NULL))
    MessageBox(hwnd, "pulser Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
    
  id=9;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "right_slider.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if (block_sprite[id] == NULL)
    MessageBox(hwnd, "right_slider Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  id=10;
  block_sprite[id]=(HBITMAP)LoadImage(NULL, "left_slider.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if (block_sprite[id] == NULL)
    MessageBox(hwnd, "left_slider Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
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
  
  
  tb_run=(HBITMAP)LoadImage(NULL, "tb_run.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_run_mask=CreateBitmapMask(tb_run, RGB(255,0,255));
  
  tb_runstop=(HBITMAP)LoadImage(NULL, "tb_runstop.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_runstop_mask=CreateBitmapMask(tb_runstop, RGB(255,0,255));
  
  tb_runstep=(HBITMAP)LoadImage(NULL, "tb_runstep.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_runstep_mask=CreateBitmapMask(tb_runstep, RGB(255,0,255));
  
  tb_push=(HBITMAP)LoadImage(NULL, "tb_push.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_push_mask=CreateBitmapMask(tb_push, RGB(255,0,255));
  
  tb_connect=(HBITMAP)LoadImage(NULL, "tb_connect.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_connect_mask=CreateBitmapMask(tb_connect, RGB(255,0,255));
  
  tb_disconnect=(HBITMAP)LoadImage(NULL, "tb_disconnect.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_disconnect_mask=CreateBitmapMask(tb_disconnect, RGB(255,0,255));
  
  tb_select=(HBITMAP)LoadImage(NULL, "tb_select.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_select_mask=CreateBitmapMask(tb_select, RGB(255,0,255));
  
  tb_nor=(HBITMAP)LoadImage(NULL, "tb_nor.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_nor_mask=CreateBitmapMask(tb_nor, RGB(255,0,255));
  
  tb_and=(HBITMAP)LoadImage(NULL, "tb_and.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_and_mask=CreateBitmapMask(tb_and, RGB(255,0,255));
  
  tb_or=(HBITMAP)LoadImage(NULL, "tb_or.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_or_mask=CreateBitmapMask(tb_or, RGB(255,0,255));
  
  tb_toggle=(HBITMAP)LoadImage(NULL, "tb_toggle.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_toggle_mask=CreateBitmapMask(tb_toggle, RGB(255,0,255));
  
  tb_pulser=(HBITMAP)LoadImage(NULL, "tb_pulser.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_pulser_mask=CreateBitmapMask(tb_pulser, RGB(255,0,255));
  
  tb_right_slider=(HBITMAP)LoadImage(NULL, "tb_right_slider.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_right_slider_mask=CreateBitmapMask(tb_right_slider, RGB(255,0,255));
  
  tb_left_slider=(HBITMAP)LoadImage(NULL, "tb_left_slider.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  tb_left_slider_mask=CreateBitmapMask(tb_left_slider, RGB(255,0,255));
     
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
    MainWindow=hwnd;

    
    /* Run the message loop. It will run until GetMessage() returns 0 */
    block *now;
    
    block *list[100000];
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
          
          
          if ((running==1)&&(execsize>0))
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
            
            blocks_move();
          }
          
          
                  
          
          frame+=1;
    }
    
    
    /* The program return-value is 0 - The value that PostQuitMessage() gave */

    
    return messages.wParam;
}

void runstep_grid()
{

    block *now;
    int i,too=0;
    block *list[100000];
     
    
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
      
   
          
    blocks_move();
                 
}



struct block* block_create(int xx,int yy,byte blocktype)
{
 struct block *lnode;

  lnode = (struct block *)malloc(sizeof(struct block));
  append_node(lnode);
  
  block_id++;
  lnode->id=block_id;
  
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
  lnode->toggle=0;
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
  
  if (blocktype==BPUSH)
  {
    lnode->draw_routine=push_draw;   
    lnode->func0_routine=push_input;    
    lnode->func1_routine=push_exec;   
    lnode->func2_routine=push_step;                  
  }
  
  if (blocktype==BNOR)
  {
    lnode->draw_routine=nor_draw;   
    lnode->func0_routine=nor_input;    
    lnode->func1_routine=nor_exec;   
    lnode->func2_routine=nor_step;                  
  }
  
  if (blocktype==BAND)
  {
    lnode->draw_routine=and_draw;   
    lnode->func0_routine=and_input;    
    lnode->func1_routine=and_exec;   
    lnode->func2_routine=and_step;                  
  }
  
  if (blocktype==BOR)
  {
    lnode->draw_routine=or_draw;   
    lnode->func0_routine=or_input;    
    lnode->func1_routine=or_exec;   
    lnode->func2_routine=or_step;                  
  }
  
  if (blocktype==BTOGGLE)
  {
    lnode->draw_routine=toggle_draw;   
    lnode->func0_routine=toggle_input;    
    lnode->func1_routine=toggle_exec;   
    lnode->func2_routine=toggle_step;                  
  }
  
  if (blocktype==BPULSER)
  {
    lnode->draw_routine=pulser_draw;   
    lnode->func0_routine=pulser_input;    
    lnode->func1_routine=pulser_exec;   
    lnode->func2_routine=pulser_step;                  
  }
  
  if (blocktype==BRIGHT_SLIDER)
  {
    lnode->draw_routine=right_slider_draw;   
    lnode->func0_routine=right_slider_input;    
    lnode->func1_routine=right_slider_exec;   
    lnode->func2_routine=right_slider_step;                  
  }
  
  if (blocktype==BLEFT_SLIDER)
  {
    lnode->draw_routine=left_slider_draw;   
    lnode->func0_routine=left_slider_input;    
    lnode->func1_routine=left_slider_exec;   
    lnode->func2_routine=left_slider_step;                  
  }
  
  return lnode;     
}


block *mouseblock;
int mousetx,mousety=0;

void mouse_event(byte event)
{
  int xx,yy,gx,gy;
  int dir;
  xx=(int)floor((mousex+view_left)/32);
  yy=(int)floor((mousey+view_top)/32);
  
  if (event==LeftDown)
  {
    if (selectedmode==0)
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
       
       grid[xx][yy]->func2_routine(grid[xx][yy]);
     }
    }
    else if ((selectedmode==1) && (grid[xx][yy]!=NULL))
    {
         
      
      if (mouseblock==NULL)
      {
        mouseblock=grid[xx][yy];
        mousetx=xx;
        mousety=yy;
                          
      }
      else
      {
        
        if (mouseblock==grid[xx][yy])  
        {
          return;                               
        }
        
        
        gx=mousetx-xx;
        gy=mousety-yy;
        
        dir=scr_xy_to_dir(gx,gy);
        
        
        if (dir!=-1)
        {
          mouseblock->connection[dir]=1;
          grid[xx][yy]->connection[scr_opposite(dir)]=1;
        }
        
        mouseblock=NULL;
        
      }
    }
  }
  
  if (event==RightDown)
  {
     if (grid[xx][yy]!=NULL)
     {
       remove_node(grid[xx][yy]);
       free(grid[xx][yy]);
       grid[xx][yy]=NULL;                
       //openlist_push(grid[xx][yy]);
       //grid[xx][yy]->movedir=2;
       //grid[xx][yy]->instack=1;
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
    
    blocks_move();
  }
  
  int i;
  if (check_key(VK_HOME)==1)
  {
    
    for(i =0;i<25000;i++)
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




void new_grid()
{
  int xx,yy,too,i;
  block *next;
  
  
  
  if (MessageBox( MainWindow,"Do you want to start a new cblock?","Cblock",MB_YESNO|MB_ICONQUESTION)==IDNO)
  {
    return;                
  }
  
  // transverus the block list deleting
  if (head!=NULL)
  {
  while(1==1)
  {
    next=head->next;
    if (next==NULL)
    {
      free(head);
      head=NULL;
      break;            
    }
    else
    {
      free(head);
    }
    head=next;
  }
  }
    
  //transverus the grid
  
  for(yy=0;yy<grid_h;yy++)
  {
    for(xx=0;xx<grid_w;xx++)                       
    {
      grid[xx][yy]=NULL;                                             
    }
  }
  
  too=execsize;
  for(i=0;i<too;i++)               
  {
    execstack_pop();
  }  
  
  selecteddirection=up; 
  selectedblock=BWIRE;
  selectedmode=0;
  
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(1, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
  
  
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(1, 0) ); 
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_LEFT ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_DOWN ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_RIGHT ,  MAKELONG(0, 0) );
  
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_SELECT ,  MAKELONG(1, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_CONNECT ,  MAKELONG(0, 0) );
  SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DISCONNECT ,  MAKELONG(0, 0) );
  
}

void open_grid()
{
  FILE *fn;
  int chr,size,i;
  fn=fopen ( "C:\\Dev-Cpp\\my projects\\srm 2011\\test0.cbl", "rb");
  byte type,outputs,dir;
  int xx,yy;
  
  size=0;
  while (1==1)
  {
    chr=fgetc(fn);
    
    if (chr==-1)
    {
      break;             
    }
    
    type=chr;//fgetc(fn);
    xx=fgetc(fn);
    yy=fgetc(fn);
    dir=fgetc(fn);
    outputs=fgetc(fn);
    
    selecteddirection=dir;
    if (type==2)
    {
      grid[xx][yy]=block_create(xx,yy,3);
      grid[xx][yy]->func0_routine(grid[xx][yy]);          
    }
    else if (type==3)
    {
      grid[xx][yy]=block_create(xx,yy,2);
      grid[xx][yy]->func0_routine(grid[xx][yy]);   
    }
    else
    {
    grid[xx][yy]=block_create(xx,yy,type);
    grid[xx][yy]->func0_routine(grid[xx][yy]);
    }
    size+=1;
  }
  
}




void srm_import_grid()
{
  FILE *fn;
  int chr,size,i;
  int width,height;
  
  int type,outputs,dir;
  int xx,yy;
  int block_list[33],posx,posy;
  posx=2;
  posy=2;
  
  fn=fopen ( "C:\\Dev-Cpp\\my projects\\srm 2011\\data.dat", "rb");
  
  size=0;
  for(i=0;i<=32;i+=1)
  {
    block_list[i]=0;
  }
  
  
  block_list[1]=BWIRE;
  block_list[4]=BNOT;
  block_list[3]=BDELTA;
  block_list[10]=BPUSH;
  block_list[8]=BNOR;
  block_list[5]=BAND;
  block_list[6]=BOR;
  block_list[32]=BTOGGLE;
  block_list[22]=BPULSER;
  block_list[19]=BRIGHT_SLIDER;
  block_list[20]=BLEFT_SLIDER;

  
  
  width=fgetc(fn);
  height=fgetc(fn);
  
  
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);
  chr=fgetc(fn);

  
  
  for(xx=0;xx<width;xx++)
  {
    for(yy=2;yy<height+2;yy++)                        
    {
      type=fgetc(fn);
      dir=fgetc(fn);
      
      if ((dir==1) || (dir==3))
      {
        dir = ( (dir+2) % 4);
      }
      else if ((dir==0) || (dir==2))
      {
        dir = ( (dir+2) % 4);
      } 
      
      
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      chr=fgetc(fn);
      
      if ((type>=0)&& (type<=32))
      {
                      
      }
      else
      {
        //runmain=0;    
        break;
      }
      
      
      if (block_list[type]!=0)
      {
        selecteddirection=dir;
        grid[xx][yy]=block_create(xx,yy,block_list[type]);
        grid[xx][yy]->func0_routine(grid[xx][yy]); 
        if ((block_list[type]==BNOT)||(block_list[type]==BNOR))
        {
          //grid[xx][yy]->func2_routine(grid[xx][yy]);                  
          execstack_push(grid[xx][yy]);
        }
        
      }

      chr=fgetc(fn);
      if (chr=-1)
      {
        //break ;           
      }
                                                   
    }
  }
  
  /*
  size=0;
  while (1==1)
  {
    chr=fgetc(fn);
    
    if (chr==-1)
    {
      break;             
    }
    
    type=chr;//fgetc(fn);
    xx=fgetc(fn);
    yy=fgetc(fn);
    dir=fgetc(fn);
    outputs=fgetc(fn);
    
    selecteddirection=dir;
    if (type==2)
    {
      grid[xx][yy]=block_create(xx,yy,3);
      grid[xx][yy]->func0_routine(grid[xx][yy]);          
    }
    else if (type==3)
    {
      grid[xx][yy]=block_create(xx,yy,2);
      grid[xx][yy]->func0_routine(grid[xx][yy]);   
    }
    else
    {
    grid[xx][yy]=block_create(xx,yy,type);
    grid[xx][yy]->func0_routine(grid[xx][yy]);
    }
    size+=1;
  }
  */
  
}





/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char string[255];
    int xx,yy,id=0;
    
    TBADDBITMAP tbab;
    TBBUTTON tbb[30];
    RECT rectTool;
    
    switch (message)                  /* handle the messages */
    {
        case WM_CREATE:
             
          
          init_block_types();
          hdcMain = GetDC(hwnd);
		  hdcBuffer = CreateCompatibleDC(hdcMain);
		  hbmBuffer = CreateCompatibleBitmap(hdcMain, window_width, window_height);
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
          
          ImageList_Add(g_hImageList,tb_run,tb_run_mask);
          ImageList_Add(g_hImageList,tb_runstop,tb_runstop_mask);
          ImageList_Add(g_hImageList,tb_runstep,tb_runstep_mask);
          
          ImageList_Add(g_hImageList,tb_push,tb_push_mask);
          
          ImageList_Add(g_hImageList,tb_connect,tb_connect_mask);
          ImageList_Add(g_hImageList,tb_disconnect,tb_disconnect_mask);
          ImageList_Add(g_hImageList,tb_select,tb_select_mask);
          
          ImageList_Add(g_hImageList,tb_nor,tb_nor_mask);//17
          
          ImageList_Add(g_hImageList,tb_and,tb_and_mask);//18
          
          ImageList_Add(g_hImageList,tb_or,tb_or_mask);//19
          
          ImageList_Add(g_hImageList,tb_toggle,tb_toggle_mask);//20
          
          ImageList_Add(g_hImageList,tb_pulser,tb_pulser_mask);//21
          
          ImageList_Add(g_hImageList,tb_right_slider,tb_right_slider_mask);//22
          
          ImageList_Add(g_hImageList,tb_left_slider,tb_left_slider_mask);//23

          
          
          SendMessage(g_hToolBar, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)g_hImageList);

          // Send the TB_BUTTONSTRUCTSIZE message, which is required for
          // backward compatibility.
          SendMessage(g_hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

          tbab.hInst = HINST_COMMCTRL;
          tbab.nID = IDB_STD_SMALL_COLOR;
          SendMessage(g_hToolBar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

          ZeroMemory(tbb, sizeof(tbb));
          
          id=0;
          
          tbb[id].iBitmap = MAKELONG(0, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_FILE_NEW;

          id+=1;

          tbb[id].iBitmap = MAKELONG(1, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_FILE_OPEN;

          id+=1;

          tbb[id].iBitmap = MAKELONG(2, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_FILE_SAVE;
          
          id+=1;

          tbb[id].fsStyle = TBSTYLE_SEP;
          
          id+=1;
          
          tbb[id].iBitmap = MAKELONG(10, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_RUN;
          
          id+=1;
          
          tbb[id].iBitmap = MAKELONG(11, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_RUNSTOP;
          
          id+=1;
          
          tbb[id].iBitmap = MAKELONG(12, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_BUTTON;
          tbb[id].idCommand = CM_RUNSTEP;
          
          id+=1;
          
          tbb[id].fsStyle = TBSTYLE_SEP;
          
          id+=1;
          
          tbb[id].iBitmap = MAKELONG(3, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_WIRE;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(4, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_NOT;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(5, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_DELTA;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(13, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_PUSH;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(17, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_NOR;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(18, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_AND;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(19,0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_OR;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(20,0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_TOGGLE;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(21,0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_PULSER;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(22,0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_RIGHT_SLIDER;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(23,0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_BLOCK_LEFT_SLIDER;
          
          id+=1;

          tbb[id].fsStyle = TBSTYLE_SEP;
                    
          id+=1;

          tbb[id].iBitmap = MAKELONG(6, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_DIRECTION_UP;
          
          id+=1;

          tbb[id].iBitmap = MAKELONG(7, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_DIRECTION_LEFT;
          
          id+=1;
          
          tbb[id].iBitmap = MAKELONG(8, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_DIRECTION_DOWN;
          
          id+=1;          
          
          tbb[id].iBitmap = MAKELONG(9, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_DIRECTION_RIGHT;
          
          id+=1;
          
          tbb[id].fsStyle = TBSTYLE_SEP;
          
          id+=1;          
          
          tbb[id].iBitmap = MAKELONG(16, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_SELECT;
          
          id+=1;          
          
          tbb[id].iBitmap = MAKELONG(14, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_CONNECT;
          
          id+=1;          
          
          tbb[id].iBitmap = MAKELONG(15, 0);
          tbb[id].fsState = TBSTATE_ENABLED;
          tbb[id].fsStyle = TBSTYLE_CHECK;
          tbb[id].idCommand = CM_DISCONNECT;
          
          


          SendMessage(g_hToolBar, TB_ADDBUTTONS, id+1, (LPARAM)&tbb);
          
          
          SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTOP, MAKELONG(0, 0));
          
          ShowWindow(g_hToolBar, TRUE);
          SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_SELECT ,  MAKELONG(1, 0) );
          SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(1, 0) );
          SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DIRECTION_UP ,  MAKELONG(1, 0) );
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
          
          
          
          for(yy=(int)floor(view_top/32);yy<(int)floor(view_top/32)+floor(window_height/tile_size);yy++)
          {
            for(xx=(int)floor(view_left/32);xx<(int)floor(view_left/32)+floor(window_width/tile_size);xx++)
            {
              //if ( (xx>=0)&&(xx<=grid_w) && (yy>=0)&&(yy<=grid_h) )
              if (grid[xx][yy]!=NULL)
              {
                //draw_bitmap(block_sprite[0],(xx*32)-view_left,(yy*32)-view_top,0);
                draw_int((xx*32)-view_left,(yy*32)-view_top,grid[xx][yy]->id,RGB(128,0,128));
                grid[xx][yy]->draw_routine(grid[xx][yy]);
              }
            }
          }
          
          
          //draw_int(60,60,10+x_lookup[3],RGB(128,0,128));
          
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
             new_grid();   
             break;
             
           case CM_FILE_OPEN:
             //open_grid();   
             srm_import_grid();
             break;
             
           case CM_RUN: 
             running=1;
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUN, MAKELONG(0, 0));
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTOP, MAKELONG(1, 0));
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTEP, MAKELONG(0, 0));
             break;
             
           case CM_RUNSTOP:
             running=0;
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUN, MAKELONG(1, 0));
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTOP, MAKELONG(0, 0));
             SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTEP, MAKELONG(1, 0));  
             break;
             
           case CM_RUNSTEP:
             runstep_grid();
             
             //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUN, MAKELONG(1, 0));
             //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTOP, MAKELONG(0, 0));
             //SendMessage(g_hToolBar, TB_ENABLEBUTTON, CM_RUNSTEP, MAKELONG(1, 0));  
             
             break;
             
           case CM_SELECT:
             selectedmode=0; 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_SELECT ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_CONNECT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DISCONNECT ,  MAKELONG(0, 0) );
             break;
             
           case CM_CONNECT:
             selectedmode=1;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_SELECT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_CONNECT ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DISCONNECT ,  MAKELONG(0, 0) );
             
             break;
             
           case CM_DISCONNECT:
             selectedmode=2; 
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_SELECT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_CONNECT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_DISCONNECT ,  MAKELONG(1, 0) );
           
             
             break;
             
           case CM_BLOCK_WIRE:
                
             selectedblock=BWIRE;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_NOT:
                
             selectedblock=BNOT;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_DELTA:
                
             selectedblock=BDELTA;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_PUSH:
                
             selectedblock=BPUSH;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_NOR:
                
             selectedblock=BNOR;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_AND:
                
             selectedblock=BAND;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_OR:
                
             selectedblock=BOR;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_TOGGLE:
                
             selectedblock=BTOGGLE;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_PULSER:
                
             selectedblock=BPULSER;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
             
           case CM_BLOCK_RIGHT_SLIDER:
                
             selectedblock=BRIGHT_SLIDER;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(1, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(0, 0) );
             break;
             
           case CM_BLOCK_LEFT_SLIDER:
                
             selectedblock=BLEFT_SLIDER;
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_WIRE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOT ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_DELTA ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PUSH ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_NOR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_AND ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_OR ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_TOGGLE ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_PULSER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_RIGHT_SLIDER ,  MAKELONG(0, 0) );
             SendMessage(g_hToolBar, TB_CHECKBUTTON, CM_BLOCK_LEFT_SLIDER ,  MAKELONG(1, 0) );
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

void draw_line(int x1,int y1,int x2,int y2,COLORREF color1)
{
  HPEN hpen=CreatePen(0,1,color1 );
  SelectObject(hdcBuffer,hpen);
  
  MoveToEx(hdcBuffer,x1,y1,NULL);
  LineTo(hdcBuffer,x2,y2);
  DeleteObject(hpen);
}

void draw_int(int x, int y, int num,COLORREF col)
{
   HFONT hfnt, hOldFont;
   char msg[10];
   sprintf(msg,"%d",num);
   int result=0;
   hfnt =(HFONT) GetStockObject(ANSI_VAR_FONT);
   result=SetBkMode(hdcBuffer,1); // 1 or 2
   col=SetTextColor(hdcBuffer,col);
	if (hOldFont =(HFONT) SelectObject(hdcBuffer, hfnt)) {
		TextOut(hdcBuffer, x, y, msg, strlen(msg));
		SelectObject(hdcMain, hOldFont);
	}
	DeleteObject(hfnt);
	DeleteObject(hOldFont);
	result=SetBkMode(hdcBuffer,0); // 1 or 2
	col=SetTextColor(hdcBuffer,RGB(0,0,0));
}

