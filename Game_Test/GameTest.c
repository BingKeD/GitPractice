#include <efi.h>
#include <Library/UefiLib.h>
#include "Protocol/CpuIo.h"
//#include "CpuIo2.h"
#include "Protocol/GraphicsOutput.h"
#include <Protocol/UgaDraw.h>

#define UP_SCANCODE		0x01
#define DOWN_SCANCODE		0x02
#define RIGHT_SCANCODE		0x03
#define LEFT_SCANCODE		0x04

#define GAME_X 400
#define GAME_Y 400

typedef struct _Element
{
	UINTN x;
	UINTN y;
	struct _Element *pre;
	struct _Element *next;
}Element;

typedef struct _Food
{
	UINTN x;
	UINTN y;
}Food;

typedef struct _SnakeData
{
	UINTN direction;
	UINTN score;
	UINTN frequency;
	Element *snake_head;
	Element *snake_tail;
}SnakeData;

extern int snake_time();
extern int snake_random();
extern void snake_srand(unsigned int seed);

EFI_SYSTEM_TABLE *my_system_table;
EFI_BOOT_SERVICES *my_boot_services;
EFI_RUNTIME_SERVICES *my_runtime_services;
SIMPLE_INPUT_INTERFACE *my_con_in;
SIMPLE_TEXT_OUTPUT_INTERFACE *my_con_out;
EFI_GRAPHICS_OUTPUT_PROTOCOL *my_draw;
EFI_CPU_IO_PROTOCOL *my_cpu_io;

EFI_GUID efi_graphics_output_protocol_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID efi_cpu_io_protocol_guid = EFI_CPU_IO_PROTOCOL_GUID;
EFI_GUID efi_cpu_io2_protocol_guid = {0xad61f191, 0xae5f, 0x4c0e, {0xb9, 0xfa, 0xe8, 0x69, 0xd2, 0x88, 0xc6, 0x4f}}; //EFI_CPU_IO2_PROTOCOL_GUID;

EFI_GRAPHICS_OUTPUT_BLT_PIXEL red={0,0,0xff,0};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL green={0,0xff,0,0};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL blue={0xff,0,0,0};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL white={0xff,0xff,0xff,0};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL black={0,0,0,0};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL foodcolor={0xff,0xff,0,0};

Food food;
SnakeData snakedata;

VOID StopTime(){
	EFI_STATUS status;
	status = my_boot_services->Stall(2000000);
}

VOID Rectangle(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *color,BOOLEAN Solid,UINTN sx,UINTN sy,UINTN lenx,UINTN leny)
{
	my_draw->Blt(my_draw,color,EfiUgaVideoFill,0,0,sx,sy,lenx,leny,0);
	if(Solid == FALSE){
		my_draw->Blt(my_draw,&black,EfiUgaVideoFill,0,0,sx+1,sy+1,lenx-2,leny-2,0);
	}
}
VOID PutFood(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *color)
{
	Rectangle(color,TRUE,food.x,food.y,10,10);
}
EFI_STATUS InitGraph(UINT32 *ScreenX,UINT32 *ScreenY)
{
	UINTN handlenum;
	EFI_STATUS status;
	EFI_HANDLE *handle_buffer;
	UINTN info_size;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINT32 xl, yl;

	status=my_boot_services->LocateHandleBuffer(ByProtocol,&efi_graphics_output_protocol_guid,NULL,&handlenum,&handle_buffer);
	if(EFI_ERROR(status)){
		my_con_out->OutputString(my_con_out,L"Locate handle buffer failly!!\r\n");
		return status;
	}


	status=my_boot_services->HandleProtocol(handle_buffer[0],&efi_graphics_output_protocol_guid,&my_draw);
	if(EFI_ERROR(status)){
		my_con_out->OutputString(my_con_out,L"Locate UGA draw protocol failly!!\r\n");
		return status;
	}

	status=my_draw->QueryMode(my_draw,my_draw->Mode->Mode,&info_size,&info);
	if(EFI_ERROR(status)){
		my_con_out->OutputString(my_con_out,L"GetMode() failly!!\n");
		return status;
	}

	xl = info->HorizontalResolution;
	yl = info->VerticalResolution;

	//Clear Screen
	my_draw->Blt(my_draw,&black,EfiUgaVideoFill,0,0,0,0,xl,yl,0);

	//Hidden the Cursor
	my_con_out->EnableCursor(my_con_out,FALSE);
	*ScreenX = xl;
	*ScreenY = yl;

	return status;
}
VOID InitGame(UINT32 ScreenX,UINT32 ScreenY)
{
	UINTN i;
	Element *tmp;
	UINT32 sx = ((ScreenX/2) - (GAME_X/2));
	UINT32 sy = ((ScreenY/2) - (GAME_Y/2));

	Rectangle(&green,TRUE,ScreenX/2-(GAME_X/2)-1,ScreenY/2-(GAME_Y/2)-1,GAME_X+2,1);

	Rectangle(&green,TRUE,ScreenX/2-(GAME_X/2)-1,ScreenY/2-(GAME_Y/2)-1,1,GAME_Y+2);

	Rectangle(&green,TRUE,ScreenX/2-(GAME_X/2)-1,ScreenY/2-(GAME_Y/2)+GAME_Y+1,GAME_X+2,1);

	Rectangle(&green,TRUE,ScreenX/2-(GAME_X/2)+GAME_X+1,ScreenY/2-(GAME_Y/2)-1,1,GAME_Y+2);

	

	snakedata.direction = RIGHT_SCANCODE;
	snakedata.score = 0;
	snakedata.frequency = 50;
	snakedata.snake_head = snakedata.snake_tail = NULL;

	Print(L"Into for loop\r\n");
	StopTime();
	for(i=0;i<10;i++){
		my_boot_services->AllocatePool(EfiBootServicesData,sizeof(Element),&tmp);
		tmp->y = (ScreenY/2) - (GAME_Y/2);
		tmp->x = ((ScreenX/2) - (GAME_X/2))+110-(i*10);
		if(i == 0){
			snakedata.snake_head = snakedata.snake_tail = tmp;
			tmp->pre = NULL;
			tmp->next = NULL;
		}else{
			snakedata.snake_tail->next = tmp;
			tmp->pre = snakedata.snake_tail;
			tmp->next = NULL;
			snakedata.snake_tail = tmp;
		}
		Rectangle(&white,FALSE,tmp->x,tmp->y,10,10);
		StopTime();
	}

	while(1){
		UINTN tmp_x = snake_random() % GAME_X;
		UINTN tmp_y = snake_random() % GAME_Y;

		tmp_x = tmp_x / 10;
		tmp_y = tmp_y / 10;

		tmp_x = tmp_x * 10;
		tmp_y = tmp_y * 10;

		food.x = sx+tmp_x;
		food.y = sy+tmp_y;

		for(tmp = snakedata.snake_head;tmp != NULL;tmp = tmp->next){
			if(tmp->x == food.x && tmp->y == food.y)
				break;
		}
		if(tmp == NULL)
			break;
	}

	PutFood(&foodcolor);
	Rectangle(&blue,TRUE,ScreenX/2-15,ScreenY/2-5,30,10);
}
VOID QuitGraph()
{
	//Display the Cursor
	my_con_out->EnableCursor(my_con_out,TRUE);
//	my_con_out->ClearScreen(my_con_out);
}
BOOLEAN SnakeMove(UINT32 ScreenX,UINT32 ScreenY)
{
	Element *tmp;
	UINT32 sx = ((ScreenX/2) - (GAME_X/2));
	UINT32 sy = ((ScreenY/2) - (GAME_Y/2));
	UINTN tail_x = snakedata.snake_tail->x;
	UINTN tail_y = snakedata.snake_tail->y;

	if(snakedata.direction == 0)
		return FALSE;

	tmp = snakedata.snake_tail;
	snakedata.snake_tail = tmp->pre;
	tmp->pre = NULL;
	snakedata.snake_tail->next = NULL;

	tmp->next = snakedata.snake_head;
	snakedata.snake_head->pre = tmp;
	snakedata.snake_head = tmp;

	if(snakedata.direction == UP_SCANCODE){
		tmp->x = tmp->next->x;
		tmp->y = tmp->next->y - 10;
	}else if(snakedata.direction == DOWN_SCANCODE){
		tmp->x = tmp->next->x;
		tmp->y = tmp->next->y + 10;
	}else if(snakedata.direction == RIGHT_SCANCODE){
		tmp->x = tmp->next->x + 10;
		tmp->y = tmp->next->y;
	}else if(snakedata.direction == LEFT_SCANCODE){
		tmp->x = tmp->next->x - 10;
		tmp->y = tmp->next->y;
	}

//	if(snakedata.snake_head->x < sx || snakedata.snake_head->y < sy){
//		return FALSE;
//	}
	if(snakedata.snake_head->x < sx){
		snakedata.snake_head->x = sx + GAME_X - 10;
	}
	if(snakedata.snake_head->y < sy){
		snakedata.snake_head->y = sy + GAME_Y - 10;
	}

//	if(snakedata.snake_head->x >= (sx+GAME_X) || snakedata.snake_head->y >= (sy+GAME_Y)){
//		return FALSE;
//	}
	if(snakedata.snake_head->x >= (sx+GAME_X)){
		snakedata.snake_head->x = sx;
	}
	if(snakedata.snake_head->y >= (sy+GAME_Y)){
		snakedata.snake_head->y = sy;
	}

	for(tmp = snakedata.snake_head->next;tmp != NULL;tmp = tmp->next){
		if(snakedata.snake_head->x == tmp->x && snakedata.snake_head->y == tmp->y){
			return FALSE;
		}
	}

	if(snakedata.snake_head->x == food.x && snakedata.snake_head->y == food.y){
		my_boot_services->AllocatePool(EfiBootServicesData,sizeof(Element),&tmp);
		tmp->x = tail_x;
		tmp->y = tail_y;
		snakedata.snake_tail ->next = tmp;
		tmp->pre = snakedata.snake_tail;
		tmp->next = NULL;
		snakedata.snake_tail = tmp;
		while(1){
			UINTN tmp_x = snake_random() % GAME_X;
			UINTN tmp_y = snake_random() % GAME_Y;

			tmp_x = tmp_x / 10;
			tmp_y = tmp_y / 10;

			tmp_x = tmp_x * 10;
			tmp_y = tmp_y * 10;

			food.x = sx+tmp_x;
			food.y = sy+tmp_y;

			for(tmp = snakedata.snake_head;tmp != NULL;tmp = tmp->next){
				if(tmp->x == food.x && tmp->y == food.y)
					break;
			}
			if(tmp == NULL)
				break;
		}
		PutFood(&foodcolor);
	}else{
		Rectangle(&black,TRUE,tail_x,tail_y,10,10);
	}

	Rectangle(&white,FALSE,snakedata.snake_head->x,snakedata.snake_head->y,10,10);

	return TRUE;
}
VOID SnakeDetect()
{
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	Status=my_boot_services->CheckEvent(my_con_in->WaitForKey);
	if(Status != EFI_SUCCESS) return;

	my_con_in->ReadKeyStroke(my_con_in,&Key);
	switch(Key.UnicodeChar){
	case 'q':
		snakedata.direction = 0;
		return;
	}
	switch(Key.ScanCode){
	case RIGHT_SCANCODE:
		if(snakedata.direction != LEFT_SCANCODE)
			snakedata.direction = RIGHT_SCANCODE;
			Print(L"RIGHT_SCANCODE\r\n");
		break;
	case LEFT_SCANCODE:
		if(snakedata.direction != RIGHT_SCANCODE)
			snakedata.direction = LEFT_SCANCODE;
			Print(L"LEFT_SCANCODE\r\n");
		break;
	case UP_SCANCODE:
		if(snakedata.direction != DOWN_SCANCODE)
			snakedata.direction = UP_SCANCODE;
			Print(L"UP_SCANCODE\r\n");
		break;
	case DOWN_SCANCODE:
		if(snakedata.direction != UP_SCANCODE)
			snakedata.direction = DOWN_SCANCODE;
			Print(L"DOWN_SCANCODE\r\n");
		break;
	}
}
EFI_STATUS SNAKEMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
	EFI_STATUS status;
	BOOLEAN Live = TRUE;
	UINT32 xl,yl;
	UINTN i;

	my_system_table=SystemTable;
	my_boot_services=SystemTable->BootServices;
	my_runtime_services=SystemTable->RuntimeServices;
	my_con_in=my_system_table->ConIn;
	my_con_out=my_system_table->ConOut;
	my_con_out->OutputString(my_con_out,L"Game starts !!\r\n");

	status=my_boot_services->LocateProtocol(&efi_cpu_io_protocol_guid,NULL,&my_cpu_io);
	if(EFI_ERROR(status)){
		status=my_boot_services->LocateProtocol(&efi_cpu_io2_protocol_guid,NULL,&my_cpu_io);
		if(EFI_ERROR(status)){
			my_con_out->OutputString(my_con_out,L"Locate CPU I/O protocol failly!!\r\n");
			return status;
		}
	}

	snake_srand(snake_time());
	status = InitGraph(&xl,&yl);
	if(EFI_ERROR(status)) return status;

	InitGame(xl,yl);

	while(Live){
		SnakeDetect();
		Print(L"SnakeDetect\r\n");
		my_boot_services->Stall(1500000);

		Live = SnakeMove(xl,yl);
		Print(L"SnakeMove\r\n");
		my_boot_services->Stall(1500000);

		for(i=0;i<snakedata.frequency;i++)
			my_boot_services->Stall(1000);
	}

	QuitGraph();
	return EFI_SUCCESS;
}
