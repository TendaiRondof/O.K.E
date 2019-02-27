//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm1 *Form1;

unsigned int A=0,R=0,G=0,B=0,RGB_Color=0xFF000000;
unsigned char R_8=0,G_8=0,B_8=0;

//Wichtig Bluetooth SPP Number
const String ServiceGUI = "{00001101-0000-1000-8000-00805F9B34FB}";  //Bluetooth SPP
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PairedDevices(void)
{
	ComboBoxPaired->Clear();
	FPairedDevices = FBluetoothManager->GetPairedDevices();
	if(FPairedDevices->Count > 0) {
		for(int i = 0; i < FPairedDevices->Count; i++) {
			ComboBoxPaired->Items->Add(FPairedDevices->Items[i]->DeviceName);
		}
	}
	else {
		ComboBoxPaired->Items->Add("No Paired Devices");
	}
}
//---------------------------------------------------------------------------
bool __fastcall TForm1::ManagerConnected(void)
{
	if(FBluetoothManager->ConnectionState == TBluetoothConnectionState::Connected) {
		Labeldiscoverable->Text = "Your Device is:  '"+
			FBluetoothManager->CurrentAdapter->AdapterName+"'";
		return true;
	}
	else {
		return false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormShow(TObject *Sender)
{
	try {
		FBluetoothManager = TBluetoothManager::Current;
		FAdapter = FBluetoothManager->CurrentAdapter;
		if(ManagerConnected()) {
			PairedDevices();
			ComboBoxPaired->ItemIndex = 0;
		}
	} catch (Exception &ex) {
		ShowMessage(ex.Message);
	}
	TabControl1->ActiveTab=TabItem1;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonConnectClick(TObject *Sender)
{
  TBytes Buffer;

  if((FSocket == NULL) || (ItemIndex != ComboBoxPaired->ItemIndex)) {
	if(ComboBoxPaired->ItemIndex > -1) {
		TBluetoothDevice * LDevice = FPairedDevices->Items[ComboBoxPaired->ItemIndex];
		FSocket = LDevice->CreateClientSocket(StringToGUID(ServiceGUI), false);
		if(FSocket != NULL) {
			ItemIndex = ComboBoxPaired->ItemIndex;
			FSocket->Connect();
		}
		else {
			ShowMessage("Out of time ~15s~");
			}
		}
		else {
			ShowMessage("No paired device selected");
		}
	}
   ButtonConnect->TintColor=TAlphaColor(claGreen);
}
//---------------------------------------------------------------------------

// Deklariert in unit1.h
//void __fastcall RGB_Send(char Item, unsigned char Value);
void __fastcall TForm1::Send(char Item, unsigned char Value)
{
 TBytes ToSend;
 unsigned char H,Z,E;
 String Senddata;

 H=Value/100;
 Z=(Value-H*100)/10;
 E=Value-H*100-Z*10;

 switch(Item)
 {
  case 'R': Senddata="R"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "R123"
			break;
  case 'G': Senddata="G"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "G009"
			break;
  case 'B': Senddata="B"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "R087"
			break;
  case 'L': Senddata="L"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "L085"
			break;
  case 'D': Senddata="D"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "D000"
			break;
  case 'X': Senddata="X"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "X000"
			break;
  case 'Y': Senddata="Y"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "Y000"
			break;
  case 'S': Senddata="S"+IntToStr(H)+IntToStr(Z)+IntToStr(E);   //z.B. "S000"
			break;
 }
 ToSend = TEncoding::ANSI->GetBytes(Senddata);
 FSocket->SendData(ToSend);
}
//---------------------------------------------------------------------------











void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
 R_8=Random(256);
 TrackBar1->Value=R_8;
 G_8=Random(256);
 TrackBar2->Value=G_8;
 B_8=Random(256);
 TrackBar3->Value=B_8;
 //Durch das Beschreiben der TRackBars wird ein Change ausgelöst,
 //Das Senden der Daten erfolgt im TrackBarChange
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
 unsigned char DIP_Switch,Joystick_X,Joystick_Y,Lichtsensor;
 TBytes ToRead;
 static int counter=0;

 Send('D',0); //"D000", Anfrage Atmel Daten
 Sleep(200); //WARTEN!!!
 ToRead=FSocket->ReadData(); //Bsp: D123X026Y115S067
 DIP_Switch=(ToRead[1]-0x30)*100+(ToRead[2]-0x30)*10+(ToRead[3]-0x30);
 if((DIP_Switch&0x80)>>7)
   Rectangle6->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle6->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x40)>>6)
   Rectangle7->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle7->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x20)>>5)
   Rectangle8->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle8->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x10)>>4)
   Rectangle9->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle9->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x08)>>3)
   Rectangle10->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle10->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x04)>>2)
   Rectangle11->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle11->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x02)>>1)
   Rectangle12->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle12->Fill->Color=TAlphaColor(0xFFE0E0E0);
if((DIP_Switch&0x01)>>0)
   Rectangle13->Fill->Color=TAlphaColor(claGreen);
 else
   Rectangle13->Fill->Color=TAlphaColor(0xFFE0E0E0);

 Send('X',0); //"X000", Anfrage Atmel Daten
 Sleep(200); //WARTEN!!!
 ToRead=FSocket->ReadData();
 Joystick_X=(ToRead[1]-0x30)*100+(ToRead[2]-0x30)*10+(ToRead[3]-0x30);
 ProgressBar1->Value=Joystick_X;
 Edit5->Text=IntToStr(Joystick_X);
 Send('Y',0); //"Y000", Anfrage Atmel Daten
 Sleep(200); //WARTEN!!!
 ToRead=FSocket->ReadData();
 Joystick_Y=(ToRead[1]-0x30)*100+(ToRead[2]-0x30)*10+(ToRead[3]-0x30);
 ProgressBar2->Value=Joystick_Y;
 Edit6->Text=IntToStr(Joystick_Y);
 Send('S',0); //"S000", Anfrage Atmel Daten
 Sleep(200); //WARTEN!!!
 ToRead=FSocket->ReadData();
 Lichtsensor=(ToRead[1]-0x30)*100+(ToRead[2]-0x30)*10+(ToRead[3]-0x30);
 ProgressBar3->Value=Lichtsensor;
 Edit8->Text=IntToStr(Lichtsensor);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1ONClick(TObject *Sender)
{
 Timer1->Enabled=true;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button2OFFClick(TObject *Sender)
{
 Timer1->Enabled=false;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar4Change(TObject *Sender)
{
 Timer1->Interval=(unsigned int)TrackBar4->Value;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar1Change(TObject *Sender)
{
//Red
 Edit2->Text=UIntToStr((unsigned int)TrackBar1->Value);
 R_8=(unsigned int)TrackBar1->Value;
 R=R_8<<16;
 RGB_Color=RGB_Color & 0xFF00FFFF; //Rot löschen
 RGB_Color=RGB_Color | R;
 Circle1->Fill->Color=TAlphaColor(RGB_Color);

 Send('R',R_8);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar2Change(TObject *Sender)
{
//Green
 Edit3->Text=UIntToStr((unsigned int)TrackBar2->Value);
 G_8=(unsigned int)TrackBar2->Value;
 G=G_8<<8;
 RGB_Color=RGB_Color & 0xFFFF00FF; //Grün löschen
 RGB_Color=RGB_Color | G;
 Circle1->Fill->Color=TAlphaColor(RGB_Color);

 Send('G',G_8);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar3Change(TObject *Sender)
{
//Blue
 Edit4->Text=UIntToStr((unsigned int)TrackBar3->Value);
 B_8=(unsigned int)TrackBar3->Value;
 B=B_8;
 RGB_Color=RGB_Color & 0xFFFFFF00; //Blau löschen
 RGB_Color=RGB_Color | B;
 Circle1->Fill->Color=TAlphaColor(RGB_Color);

 Send('B',B_8);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TrackBar5Change(TObject *Sender)
{
 unsigned char LED_Port;
 Edit7->Text=UIntToStr((unsigned int)TrackBar5->Value);
 LED_Port=(unsigned int)TrackBar5->Value;
 Send('L',LED_Port);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{
   Timer2->Enabled=true;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button2Click(TObject *Sender)
{
   Timer2->Enabled=false;
}
//---------------------------------------------------------------------------


