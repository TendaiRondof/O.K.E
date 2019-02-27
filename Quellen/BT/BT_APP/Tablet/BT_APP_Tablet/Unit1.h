//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.Memo.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <System.Bluetooth.hpp>
#include <FMX.TabControl.hpp>
#include <FMX.Objects.hpp>
//---------------------------------------------------------------------------

class TForm1;

class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TTabControl *TabControl1;
	TTabItem *TabItem1;
	TButton *ButtonConnect;
	TComboBox *ComboBoxPaired;
	TLabel *Labeldiscoverable;
	TLabel *LabelPaired;
	TTabItem *TabItem2;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TLabel *Label4;
	TImage *Image1;
	TRectangle *Rectangle1;
	TLabel *Label13;
	TButton *Button1ON;
	TTrackBar *TrackBar4;
	TLabel *Label5;
	TButton *Button2OFF;
	TCircle *Circle1;
	TLabel *Label8;
	TLabel *Label7;
	TLabel *Label6;
	TTrackBar *TrackBar1;
	TTrackBar *TrackBar2;
	TTrackBar *TrackBar3;
	TEdit *Edit2;
	TEdit *Edit3;
	TEdit *Edit4;
	TLabel *Label14;
	TRectangle *Rectangle2;
	TLabel *Label12;
	TTrackBar *TrackBar5;
	TEdit *Edit7;
	TLabel *Label15;
	TRectangle *Rectangle4;
	TLabel *Label11;
	TLabel *Label16;
	TLabel *Label17;
	TProgressBar *ProgressBar1;
	TProgressBar *ProgressBar2;
	TProgressBar *ProgressBar3;
	TEdit *Edit5;
	TEdit *Edit6;
	TEdit *Edit8;
	TLabel *Label18;
	TRectangle *Rectangle5;
	TRectangle *Rectangle6;
	TRectangle *Rectangle7;
	TRectangle *Rectangle8;
	TRectangle *Rectangle9;
	TRectangle *Rectangle10;
	TRectangle *Rectangle11;
	TRectangle *Rectangle12;
	TRectangle *Rectangle13;
	TLabel *Label24;
	TLabel *Label23;
	TLabel *Label22;
	TLabel *Label21;
	TLabel *Label20;
	TLabel *Label19;
	TLabel *Label25;
	TLabel *Label26;
	TButton *Button1;
	TButton *Button2;
	TImage *Image3;
	TTimer *Timer1;
	TTimer *Timer2;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall ButtonConnectClick(TObject *Sender);
    void __fastcall Send(char Item, unsigned char value);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall Timer2Timer(TObject *Sender);
	void __fastcall Button1ONClick(TObject *Sender);
	void __fastcall Button2OFFClick(TObject *Sender);
	void __fastcall TrackBar4Change(TObject *Sender);
	void __fastcall TrackBar1Change(TObject *Sender);
	void __fastcall TrackBar2Change(TObject *Sender);
	void __fastcall TrackBar3Change(TObject *Sender);
	void __fastcall TrackBar5Change(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
	void __fastcall Button2Click(TObject *Sender);
private:	// User declarations
	TBluetoothManager * FBluetoothManager;
	TBluetoothDeviceList *FDiscoverDevices;
	TBluetoothDeviceList * FPairedDevices;
	TBluetoothAdapter * FAdapter;
	TBytes FData;
	TBluetoothSocket * FSocket;
	int ItemIndex;
	void __fastcall PairedDevices(void);
	bool __fastcall ManagerConnected(void);
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
