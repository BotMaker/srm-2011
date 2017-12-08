#include <windows.h>
#include <stdio.h>
#include <math.h>

void draw_bitmap(HBITMAP image,int x,int y,byte image_single);
void draw_rectangle(int x1,int y1,int x2,int y2,COLORREF color1,COLORREF color2, int fill);
void drawstring2(int x, int y, const char *msg,COLORREF col,int w,int h);
struct block;
void wire_draw(block *lnode);


bool runmain=1;

HBITMAP hbmBuffer =NULL;
HBITMAP hbmOldBuffer=NULL;
HDC hdcBuffer = NULL;
HDC hdcMain=NULL;

int window_width=640;
int window_height=480;
int view_left,view_top=0;

const int grid_w=100;
const int grid_h=100;

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

int block_types[32][4];
void *block_draw[32];

struct block *head, *tail;

void append_node(struct dllist *lnode);
void insert_node(struct dllist *lnode, struct dllist *after);
void remove_node(struct dllist *lnode);

struct block *grid[grid_w][grid_h];



void init_block_types()
{
  block_types[1][0]=BWIRE; // wire
  block_types[1][1]=0; //sprite index
  
  block_types[2][0]=BNOT; // wire
  block_types[2][1]=1; //sprite index

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
  bool connection[4];
  bool instack;
  byte movedir;
  byte settings;
/*
void *event0 pointer   //c++ game event engine.c
void *event1 pointer
void *event2 pointer
void *event3 pointer
void *draw pointer
*/

  bool input;

  int blocktype;
  
  byte image_single;
  byte sprite_index;
  
  void (*draw_routine)(block *lnode);
  void (*func0_routine)(block *lnode);
  void (*func1_routine)(block *lnode);
  void (*func2_routine)(block *lnode);
  // could use a array of hbitmaps and index into the array
  //from a 4byte pointer to a 1 byte index
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
      nnn=grid[lnode->xx-1][lnode->yy-1];
       
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);             
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
      nnn=grid[lnode->xx-1][lnode->yy-1];
       
      scr_exec_push(grid[ lnode->xx+1][lnode->yy],1);
      break;
  };   
  
  if (nnn!=NULL)
  {
    lnode->input=bit4_test(nnn->outputs,lnode->direction);             
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




void block_sprite_load(HWND hwnd)
{
  block_sprite[0]=(HBITMAP)LoadImage(NULL, "wire.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[0]=(HBITMAP)LoadImage(NULL, "wire_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[0] == NULL) || (block_sprite_on[0] == NULL))
    MessageBox(hwnd, "wire Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);
    
  block_sprite[1]=(HBITMAP)LoadImage(NULL, "not.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  block_sprite_on[1]=(HBITMAP)LoadImage(NULL, "not_on.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
  if ((block_sprite[1] == NULL) || (block_sprite_on[1] == NULL))
    MessageBox(hwnd, "not Could not load bitmap!", "Error", MB_OK | MB_ICONEXCLAMATION);    
}


/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
char szClassName[ ] = "WindowsApp";

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

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
            now=execstack_pop();
            now->func2_routine(now);          
          }
          
          
          frame+=1;
    }
    
    
    /* The program return-value is 0 - The value that PostQuitMessage() gave */

    
    return messages.wParam;
}

byte selectedblock=BWIRE;

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
  lnode->direction=0;
  lnode->data=0;
  lnode->image_single=0;
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

void keyboard_event()
{
     
  if (check_key(VK_SPACE)==1)
  {
    selectedblock=BWIRE;                        
  }
  if (check_key(VK_TAB)==1)
  {
    selectedblock=BNOT;                        
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
		  break;
		  
		  
        case  WM_MOUSEMOVE:
              
          mousex = lParam & 0xffff;
          mousey = lParam >> 16;
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
		  
        case WM_TIMER:
             
          fps=frame;
          
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
          BitBlt(hdcMain, 0, 0, window_width , window_height , hdcBuffer, 0, 0, SRCCOPY);
          frame=0;
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
