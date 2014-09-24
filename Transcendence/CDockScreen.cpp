//	CDockScreen.cpp
//
//	CDockScreen class

#include "PreComp.h"
#include "Transcendence.h"

const int g_cyDockScreen = 528;
const int g_cxBackground = 1024;
const int g_cyBackground = 528;
const int DESC_PANE_X =			600;
const int BACKGROUND_FOCUS_X =	(DESC_PANE_X / 2);
const int BACKGROUND_FOCUS_Y =	(g_cyBackground / 2);
const int MAX_SCREEN_WIDTH =	1024;
const int MAX_BACKGROUND_WIDTH =	1280;
const int EXTRA_BACKGROUND_IMAGE = 128;

const int STATUS_BAR_HEIGHT	=	20;
const int g_cyTitle =			72;
const int g_cxActionsRegion =	400;

const int g_cyItemTitle =		32;
const int g_cxItemMargin =		132;
const int g_cxItemImage =		96;
const int g_cyItemImage =		96;

const int g_cxStats =			400;
const int g_cyStats =			30;
const int g_cxCargoStats =		200;
const int g_cxCargoStatsLabel =	100;

const int g_FirstActionID =		100;
const int g_LastActionID =		199;

const int g_PrevActionID =		200;
const int g_NextActionID =		201;
const int g_ItemTitleID =		202;
const int g_ItemDescID =		203;
const int g_CounterID =			204;
const int g_ItemImageID =		205;
const int TEXT_INPUT_ID =		207;
const int IMAGE_AREA_ID =		208;
const int DISPLAY_ID =			209;

const int ACTION_CUSTOM_NEXT_ID =	300;
const int ACTION_CUSTOM_PREV_ID =	301;

#define CANVAS_TAG					CONSTLIT("Canvas")
#define DISPLAY_TAG					CONSTLIT("Display")
#define GROUP_TAG					CONSTLIT("Group")
#define IMAGE_TAG					CONSTLIT("Image")
#define INITIAL_PANE_TAG			CONSTLIT("InitialPane")
#define INITIALIZE_TAG				CONSTLIT("Initialize")
#define NEUROHACK_TAG				CONSTLIT("Neurohack")
#define ON_DISPLAY_INIT_TAG			CONSTLIT("OnDisplayInit")
#define ON_INIT_TAG					CONSTLIT("OnInit")
#define ON_PANE_INIT_TAG			CONSTLIT("OnPaneInit")
#define ON_SCREEN_INIT_TAG			CONSTLIT("OnScreenInit")
#define ON_SCREEN_UPDATE_TAG		CONSTLIT("OnScreenUpdate")
#define PANES_TAG					CONSTLIT("Panes")
#define TEXT_TAG					CONSTLIT("Text")

#define ALIGN_ATTRIB				CONSTLIT("align")
#define ANIMATE_ATTRIB				CONSTLIT("animate")
#define BACKGROUND_ID_ATTRIB		CONSTLIT("backgroundID")
#define BOTTOM_ATTRIB				CONSTLIT("bottom")
#define CENTER_ATTRIB				CONSTLIT("center")
#define COLOR_ATTRIB				CONSTLIT("color")
#define DESC_ATTRIB					CONSTLIT("desc")
#define FONT_ATTRIB					CONSTLIT("font")
#define HEIGHT_ATTRIB				CONSTLIT("height")
#define ID_ATTRIB					CONSTLIT("id")
#define LEFT_ATTRIB					CONSTLIT("left")
#define NAME_ATTRIB					CONSTLIT("name")
#define NO_LIST_NAVIGATION_ATTRIB	CONSTLIT("noListNavigation")
#define PANE_ATTRIB					CONSTLIT("pane")
#define RIGHT_ATTRIB				CONSTLIT("right")
#define SHOW_TEXT_INPUT_ATTRIB		CONSTLIT("showTextInput")
#define TOP_ATTRIB					CONSTLIT("top")
#define TRANSPARENT_ATTRIB			CONSTLIT("transparent")
#define TYPE_ATTRIB					CONSTLIT("type")
#define VALIGN_ATTRIB				CONSTLIT("valign")
#define VCENTER_ATTRIB				CONSTLIT("vcenter")
#define WIDTH_ATTRIB				CONSTLIT("width")

#define SCREEN_TYPE_ARMOR_SELECTOR	CONSTLIT("armorSelector")
#define SCREEN_TYPE_CUSTOM_PICKER	CONSTLIT("customPicker")
#define SCREEN_TYPE_CUSTOM_ITEM_PICKER	CONSTLIT("customItemPicker")
#define SCREEN_TYPE_ITEM_PICKER		CONSTLIT("itemPicker")

#define ALIGN_CENTER				CONSTLIT("center")
#define ALIGN_RIGHT					CONSTLIT("right")
#define ALIGN_LEFT					CONSTLIT("left")
#define ALIGN_BOTTOM				CONSTLIT("bottom")
#define ALIGN_TOP					CONSTLIT("top")
#define ALIGN_MIDDLE				CONSTLIT("middle")

#define BAR_COLOR							CG16bitImage::RGBValue(0, 2, 10)

CDockScreen::CDockScreen (void) : 
		m_pFonts(NULL),
		m_pPlayer(NULL),
		m_pLocation(NULL),
		m_pData(NULL),
		m_pDesc(NULL),
		m_pScreen(NULL),
		m_bFirstOnInit(true),
		m_bInOnInit(false),

		m_pBackgroundImage(NULL),
		m_pCredits(NULL),
		m_pCargoSpace(NULL),
		m_bFreeBackgroundImage(false),

		m_pDisplayInitialize(NULL),
		m_bDisplayAnimate(false),
		m_pDisplay(NULL),

		m_pPanes(NULL),
		m_pOnScreenUpdate(NULL)

//	CDockScreen constructor

	{
	}

CDockScreen::~CDockScreen (void)

//	CDockScreen destructor

	{
	CleanUpScreen();
	}

void CDockScreen::Action (DWORD dwTag, DWORD dwData)

//	Action
//
//	Button pressed

	{
	IDockScreenDisplay::EResults iResult = m_pDisplay->HandleAction(dwTag, dwData);
	
	switch (iResult)
		{
		//	If handled, then we're done

		case IDockScreenDisplay::resultHandled:
			return;

		//	If we need to reshow the pane, do it.

		case IDockScreenDisplay::resultShowPane:
			m_CurrentPane.ExecuteShowPane(EvalInitialPane());
			return;
		}

	//	Otherwise, invoke the button

	m_CurrentPane.HandleAction(dwTag, dwData);
	}

void CDockScreen::AddDisplayControl (CXMLElement *pDesc, 
									 AGScreen *pScreen, 
									 SDisplayControl *pParent,
									 const RECT &rcFrame, 
									 SDisplayControl **retpDControl)

//	AddDisplayControl
//
//	Adds a control from XML element

	{
	int i;
	CCodeChain &CC = g_pUniverse->GetCC();
	SDisplayControl *pDControl = m_Controls.Insert();

	//	Set basic attribs

	pDControl->sID = pDesc->GetAttribute(ID_ATTRIB);

	//	Propage animation from parent, unless overridden.
	//	Note: we default to always animate

	if (!pDesc->FindAttributeBool(ANIMATE_ATTRIB, &pDControl->bAnimate))
		pDControl->bAnimate = (pParent ? pParent->bAnimate : true);

	//	Control rect (relative to pScreen)

	RECT rcRect;
	InitDisplayControlRect(pDesc, rcFrame, &rcRect);

	//	Get the font

	const CG16bitFont *pControlFont = &m_pFonts->Large;
	CString sFontName;
	if (pDesc->FindAttribute(FONT_ATTRIB, &sFontName))
		pControlFont = &GetFontByName(*m_pFonts, sFontName);

	WORD wControlColor;
	CString sColorName;
	if (pDesc->FindAttribute(COLOR_ATTRIB, &sColorName))
		wControlColor = ::LoadRGBColor(sColorName);
	else
		wControlColor = CG16bitImage::RGBValue(255, 255, 255);

	//	Create the control based on the type

	if (strEquals(pDesc->GetTag(), TEXT_TAG))
		{
		pDControl->iType = ctrlText;

		CGTextArea *pControl = new CGTextArea;
		pControl->SetFont(pControlFont);
		pControl->SetColor(wControlColor);

		CString sAlign = pDesc->GetAttribute(ALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			pControl->SetStyles(alignCenter);
		else if (strEquals(sAlign, ALIGN_RIGHT))
			pControl->SetStyles(alignRight);

		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the text code

		pDControl->pCode = CC.Link(pDesc->GetContentText(0), 0, NULL);
		}
	else if (strEquals(pDesc->GetTag(), IMAGE_TAG))
		{
		pDControl->iType = ctrlImage;

		CGImageArea *pControl = new CGImageArea;
		pControl->SetTransBackground(pDesc->GetAttributeBool(TRANSPARENT_ATTRIB));

		DWORD dwStyles = 0;
		CString sAlign = pDesc->GetAttribute(ALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			dwStyles |= alignCenter;
		else if (strEquals(sAlign, ALIGN_RIGHT))
			dwStyles |= alignRight;
		else
			dwStyles |= alignLeft;

		sAlign = pDesc->GetAttribute(VALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			dwStyles |= alignMiddle;
		else if (strEquals(sAlign, ALIGN_BOTTOM))
			dwStyles |= alignBottom;
		else
			dwStyles |= alignTop;

		pControl->SetStyles(dwStyles);

		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the code that returns the image

		pDControl->pCode = CC.Link(pDesc->GetContentText(0), 0, NULL);
		}
	else if (strEquals(pDesc->GetTag(), CANVAS_TAG))
		{
		pDControl->iType = ctrlCanvas;

		CGDrawArea *pControl = new CGDrawArea;
		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the draw code

		pDControl->pCode = CC.Link(pDesc->GetContentText(0), 0, NULL);
		}
	else if (strEquals(pDesc->GetTag(), GROUP_TAG))
		{
		for (i = 0; i < pDesc->GetContentElementCount(); i++)
			{
			CXMLElement *pSubDesc = pDesc->GetContentElement(i);
			AddDisplayControl(pSubDesc, pScreen, pDControl, rcRect);
			}
		}
	else if (strEquals(pDesc->GetTag(), NEUROHACK_TAG))
		{
		pDControl->iType = ctrlNeurohack;

		CGNeurohackArea *pControl = new CGNeurohackArea;
		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the text code

		pDControl->pCode = CC.Link(pDesc->GetContentText(0), 0, NULL);
		}

	//	Done

	if (retpDControl)
		*retpDControl = pDControl;
	}

void CDockScreen::CleanUpScreen (void)

//	CleanUpScreen
//
//	Called to bring the screen down after InitScreen

	{
	if (m_pScreen)
		{
		delete m_pScreen;
		m_pScreen = NULL;
		}

	if (m_pBackgroundImage)
		{
		if (m_bFreeBackgroundImage)
			delete m_pBackgroundImage;

		m_pBackgroundImage = NULL;
		}

	if (m_pDisplay)
		{
		delete m_pDisplay;
		m_pDisplay = NULL;
		}

	m_Controls.DeleteAll();
	m_pDisplayInitialize = NULL;
	m_pData = NULL;

	m_CurrentPane.CleanUp();

	if (m_pOnScreenUpdate)
		{
		m_pOnScreenUpdate->Discard(&g_pUniverse->GetCC());
		m_pOnScreenUpdate = NULL;
		}
	}

ALERROR CDockScreen::CreateBackgroundArea (CXMLElement *pDesc, AGScreen *pScreen, const RECT &rcRect, const RECT &rcInner)

//	CreateBackgroundArea
//
//	Creates the background area

	{
	//	Generate a background image

	CreateBackgroundImage(pDesc, rcRect, rcInner.left - rcRect.left);

	//	Create the area

	if (m_pBackgroundImage)
		{
		//	Add the background

		RECT rcBackArea;
		CGImageArea *pImage = new CGImageArea;

		RECT rcImage;
		rcImage.left = 0;
		rcImage.top = 0;
		rcImage.right = m_pBackgroundImage->GetWidth();
		rcImage.bottom = m_pBackgroundImage->GetHeight();
		pImage->SetImage(m_pBackgroundImage, rcImage);

		rcBackArea.left = rcRect.left;
		rcBackArea.top = (RectHeight(rcRect) - g_cyDockScreen) / 2;
		rcBackArea.right = rcBackArea.left + RectWidth(rcRect);
		rcBackArea.bottom = rcBackArea.top + m_pBackgroundImage->GetHeight();

		//	bSendToBack = true because we may have created other areas before 
		//	this and we need the background to be in back.

		pScreen->AddArea(pImage, rcBackArea, IMAGE_AREA_ID, true);
		}

	return NOERROR;
	}

ALERROR CDockScreen::CreateBackgroundImage (CXMLElement *pDesc, const RECT &rcRect, int xOffset)

//	CreateBackgroundImage
//
//	Creates the background image to use for the dock screen. Initializes
//	m_pBackgroundImage and m_bFreeBackgroundImage

	{
	enum BackgroundTypes { backgroundNone, backgroundImage, backgroundObj };

	ASSERT(m_pLocation);
	ASSERT(m_pBackgroundImage == NULL);

	BackgroundTypes iType = backgroundNone;
	DWORD dwBackgroundID = 0;
	CSpaceObject *pBackgroundObj = m_pLocation;
	int cxBackground = RectWidth(rcRect);
	int cyBackground = g_cyBackground;

	//	Figure out which background to use

	CString sBackgroundID;
	if (pDesc->FindAttribute(BACKGROUND_ID_ATTRIB, &sBackgroundID))
		{
		//	If the attribute exists, but is empty (or equals "none") then
		//	we don't have a background

		if (sBackgroundID.IsBlank() || strEquals(sBackgroundID, CONSTLIT("none")))
			iType = backgroundNone;

		//	If the ID is "object" then we should ask the object

		else if (strEquals(sBackgroundID, CONSTLIT("object")))
			iType = backgroundObj;

		//	Otherwise, we expect an integer

		else
			{
			iType = backgroundImage;
			dwBackgroundID = strToInt(sBackgroundID, 0);
			}
		}

	//	If no attribute specified, ask the display

	else
		{
		if (m_pDisplay->GetDefaultBackgroundObj(&pBackgroundObj))
			iType = backgroundObj;
		else
			{
			pBackgroundObj = m_pLocation;
			dwBackgroundID = pBackgroundObj->GetDefaultBkgnd();
			iType = (dwBackgroundID ? backgroundImage : backgroundObj);
			}
		}

	//	Load the image

	CG16bitImage *pImage = NULL;
	if (dwBackgroundID)
		pImage = g_pUniverse->GetLibraryBitmap(dwBackgroundID);

	//	Sometimes (like in the case of item lists) the image is larger than normal

	int cyExtra = 0;
	if (pImage)
		cyExtra = Max(pImage->GetHeight() - cyBackground, 0);

	//	Create a new image for the background

	m_pBackgroundImage = new CG16bitImage;
	m_bFreeBackgroundImage = true;
	m_pBackgroundImage->CreateBlank(cxBackground, cyBackground + cyExtra, false);

	if (cyExtra)
		m_pBackgroundImage->Fill(0, cyBackground, cxBackground, cyExtra, 0);

	//	Load the dock screen background based on the ship class

	DWORD dwScreenUNID = DEFAULT_DOCK_SCREEN_IMAGE_UNID;
	DWORD dwScreenMaskUNID = DEFAULT_DOCK_SCREEN_MASK_UNID;
	CG16bitImage *pScreenImage = g_pUniverse->GetLibraryBitmap(dwScreenUNID);

	//	Blt to background

	if (pScreenImage)
		{
		//	Right-align the image on the screen
		int xOffset = cxBackground - pScreenImage->GetWidth();
		m_pBackgroundImage->Blt(0, 0, pScreenImage->GetWidth(), pScreenImage->GetHeight(), *pScreenImage, xOffset, 0);
		}

	//	If not image, then we're done

	if (iType == backgroundNone)
		;

	//	Paint the object as the background

	else if (iType == backgroundObj)
		{
		//	If this is the player ship then we draw a large image

		CShip *pShip = pBackgroundObj->AsShip();
		CShipClass *pClass = (pShip ? pShip->GetClass() : NULL);
		const CPlayerSettings *pPlayer = (pClass ? pClass->GetPlayerSettings() : NULL);
		const CG16bitImage *pLargeImage = (pPlayer ? g_pUniverse->GetLibraryBitmap(pPlayer->GetLargeImage()) : NULL);

		if (pLargeImage && !pLargeImage->IsEmpty())
			{
			if (pLargeImage->GetHeight() < cyBackground)
				{
				m_pBackgroundImage->ColorTransBlt(0,
						0,
						pLargeImage->GetWidth(),
						pLargeImage->GetHeight(),
						255,
						*pLargeImage,
						xOffset + BACKGROUND_FOCUS_X - (pLargeImage->GetWidth() / 2),
						BACKGROUND_FOCUS_Y - (pLargeImage->GetHeight() / 2));
				}
			else
				{
				Metric rScale = (Metric)cyBackground / pLargeImage->GetHeight();
				CG16bitImage *pNewImage = new CG16bitImage;
				pNewImage->CreateFromImageTransformed(*pLargeImage,
						0,
						0,
						pLargeImage->GetWidth(),
						pLargeImage->GetHeight(),
						rScale,
						rScale,
						0.0);

				m_pBackgroundImage->ColorTransBlt(0,
						0,
						pNewImage->GetWidth(),
						pNewImage->GetHeight(),
						255,
						*pNewImage,
						xOffset + BACKGROUND_FOCUS_X - (pNewImage->GetWidth() / 2),
						BACKGROUND_FOCUS_Y - (pNewImage->GetHeight() / 2));

				delete pNewImage;
				}
			}

		//	Otherwise we draw the object

		else
			{
			SViewportPaintCtx Ctx;
			Ctx.fNoSelection = true;
			Ctx.pObj = m_pLocation;
			pBackgroundObj->Paint(*m_pBackgroundImage,
					xOffset + BACKGROUND_FOCUS_X,
					BACKGROUND_FOCUS_Y,
					Ctx);
			}
		}

	//	If we have an image with a mask, just blt the masked image

	else if (pImage && pImage->HasMask())
		m_pBackgroundImage->ColorTransBlt(0, 0, pImage->GetWidth(), pImage->GetHeight(), 255, *pImage, xOffset, 0);

	//	If we have an image with no mask, then we need to create our own mask.
	//	If the image is larger than the space, then it is flush right with 
	//	the center line. Otherwise, it is flush left.

	else if (pImage)
		{
		//	EXTRA_BACKGROUND_IMAGE is the amount of image to show to the
		//	right of the center-line.

		int cxAvail = (RectWidth(rcRect) / 2) + EXTRA_BACKGROUND_IMAGE;
		int xImage = -Max(0, pImage->GetWidth() - cxAvail);

		CG16bitImage *pScreenMask = g_pUniverse->GetLibraryBitmap(dwScreenMaskUNID);
		if (pScreenMask)
			{
			//	Center the mask and align it with the position of the background.

			int xAlpha = Max(0, (pScreenMask->GetWidth() - m_pBackgroundImage->GetWidth()) / 2);

			//	If the image is too small, then slide the mask over to the right
			//	so that the fade-out part aligns with the right edge of the image.

			if (pImage->GetWidth() < cxAvail)
				xAlpha += (cxAvail - pImage->GetWidth());

			//	Generate a new bitmap containing the mask at the exact position of
			//	the image we want to blt.

			CG16bitImage Mask;
			if (xAlpha != 0)
				{
				Mask.CreateBlankAlpha(pImage->GetWidth(), pImage->GetHeight());
				Mask.CopyAlpha(xAlpha, 0, pImage->GetWidth(), pImage->GetHeight(), *pScreenMask, 0, 0);
				pScreenMask = &Mask;
				}

			m_pBackgroundImage->BltWithMask(0, 0, pImage->GetWidth(), pImage->GetHeight(), *pScreenMask, *pImage, xImage, 0);
			}
		else
			m_pBackgroundImage->Blt(xImage, 0, pImage->GetWidth(), pImage->GetHeight(), *pImage, 0, 0);
		}

	return NOERROR;
	}

ALERROR CDockScreen::CreateTitleArea (CXMLElement *pDesc, AGScreen *pScreen, const RECT &rcRect, const RECT &rcInner)

//	CreateTitleArea
//
//	Creates the title and status bar

	{
	const CVisualPalette &VI = g_pHI->GetVisuals();

	int yTop = (RectHeight(rcRect) - g_cyDockScreen) / 2;

	//	Add a background bar to the title part

	CGImageArea *pImage = new CGImageArea;
	pImage->SetBackColor(VI.GetColor(colorAreaDockTitle));
	RECT rcArea;
	rcArea.left = rcRect.left;
	rcArea.top = yTop - g_cyTitle;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop - STATUS_BAR_HEIGHT;
	pScreen->AddArea(pImage, rcArea, 0);

	pImage = new CGImageArea;
	pImage->SetBackColor(CG16bitImage::DarkenPixel(VI.GetColor(colorAreaDockTitle), 200));
	rcArea.left = rcRect.left;
	rcArea.top = yTop - STATUS_BAR_HEIGHT;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop;
	pScreen->AddArea(pImage, rcArea, 0);

	//	Get the name of this location

	CString sName;
	if (!EvalString(pDesc->GetAttribute(NAME_ATTRIB), m_pData, false, eventNone, &sName))
		{
		ReportError(strPatternSubst(CONSTLIT("Error evaluating location name: %s"), sName));
		sName = NULL_STR;
		}
		
	if (sName.IsBlank())
		sName = m_pLocation->GetNounPhrase(nounTitleCapitalize);

	//	Add the name as a title to the screen

	CGTextArea *pText = new CGTextArea;
	pText->SetText(sName);
	pText->SetFont(&m_pFonts->Title);
	pText->SetColor(VI.GetColor(colorTextDockTitle));
	pText->AddShadowEffect();
	rcArea.left = rcRect.left + 8;
	rcArea.top = yTop - g_cyTitle;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop;
	pScreen->AddArea(pText, rcArea, 0);

	//	Add the money area

	int cyOffset = (STATUS_BAR_HEIGHT - m_pFonts->MediumHeavyBold.GetHeight()) / 2;

	m_pCredits = new CGTextArea;
	m_pCredits->SetFont(&m_pFonts->MediumHeavyBold);
	m_pCredits->SetColor(VI.GetColor(colorTextDockTitle));

	rcArea.left = rcInner.right - g_cxStats;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(m_pCredits, rcArea, 0);

	//	Add the cargo space label

	pText = new CGTextArea;
	pText->SetText(CONSTLIT("Cargo Space:"));
	pText->SetFont(&m_pFonts->MediumHeavyBold);
	pText->SetColor(VI.GetColor(colorTextDockTitle));

	rcArea.left = rcInner.right - g_cxCargoStats;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(pText, rcArea, 0);

	//	Add the cargo space area

	m_pCargoSpace = new CGTextArea;
	m_pCargoSpace->SetFont(&m_pFonts->MediumHeavyBold);
	m_pCargoSpace->SetColor(VI.GetColor(colorTextDockTitle));

	rcArea.left = rcInner.right - g_cxCargoStats + g_cxCargoStatsLabel;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(m_pCargoSpace, rcArea, 0);

	UpdateCredits();

	return NOERROR;
	}

ICCItem *CDockScreen::GetCurrentListEntry (void)

//	GetCurrentListEntry
//
//	Returns the current list entry

	{
	ICCItem *pResult = m_pDisplay->GetCurrentListEntry();
	if (pResult == NULL)
		return g_pUniverse->GetCC().CreateNil();

	return pResult;
	}

CG16bitImage *CDockScreen::GetDisplayCanvas (const CString &sID)

//	GetDisplayCanvas
//
//	Returns the given canvas (NULL if none found)

	{
	SDisplayControl *pControl = FindDisplayControl(sID);
	if (pControl == NULL || pControl->pArea == NULL)
		return NULL;

	if (pControl->iType != ctrlCanvas)
		return NULL;

	CGDrawArea *pCanvasControl = (CGDrawArea *)pControl->pArea;
	return &pCanvasControl->GetCanvas();
	}

CDesignType *CDockScreen::GetResolvedRoot (CString *retsResolveScreen) const

//	GetResolvedRoot
//
//	Returns the screen root for the current stack frame.

	{
	SDockFrame CurFrame;
	g_pTrans->GetModel().GetScreenSession(&CurFrame);

	if (retsResolveScreen)
		*retsResolveScreen = CurFrame.sResolvedScreen;

	return CurFrame.pResolvedRoot;
	}

bool CDockScreen::EvalBool (const CString &sCode)

//	EvalBool
//
//	Evaluates the given CodeChain code.

	{
	CCodeChainCtx Ctx;
	Ctx.SetScreen(this);
	Ctx.SaveAndDefineSourceVar(m_pLocation);
	Ctx.SaveAndDefineDataVar(m_pData);

	char *pPos = sCode.GetPointer();
	ICCItem *pExp = Ctx.Link(sCode, 1, NULL);

	ICCItem *pResult = Ctx.Run(pExp);	//	LATER:Event
	Ctx.Discard(pExp);

	if (pResult->IsError())
		{
		SetDescription(pResult->GetStringValue());
		kernelDebugLogMessage(pResult->GetStringValue());
		}

	bool bResult = !pResult->IsNil();
	Ctx.Discard(pResult);

	return bResult;
	}

CString CDockScreen::EvalInitialPane (void)

//	EvalInitialPane
//
//	Invokes <InitialPane>

	{
	SDockFrame CurFrame;
	g_pTrans->GetModel().GetScreenSession(&CurFrame);
	return EvalInitialPane(CurFrame.pLocation, CurFrame.pInitialData);
	}

CString CDockScreen::EvalInitialPane (CSpaceObject *pSource, ICCItem *pData)

//	EvalInitialPane
//
//	Invokes <InitialPane> code and returns the result (or "Default")

	{
	CXMLElement *pInitialPane = m_pDesc->GetContentElementByTag(INITIAL_PANE_TAG);
	if (pInitialPane)
		{
		CString sPane;
		CString sCode = pInitialPane->GetContentText(0);

		//	Execute

		CCodeChainCtx Ctx;
		Ctx.SetScreen(this);
		Ctx.SaveAndDefineSourceVar(pSource);
		Ctx.SaveAndDefineDataVar(pData);

		ICCItem *pExp = Ctx.Link(sCode, 0, NULL);

		ICCItem *pResult = Ctx.Run(pExp);	//	LATER:Event
		Ctx.Discard(pExp);

		if (pResult->IsError())
			{
			CString sError = pResult->GetStringValue();
			::kernelDebugLogMessage(sError);
			SetDescription(sError);
			}
		else
			sPane = pResult->GetStringValue();

		Ctx.Discard(pResult);
		return sPane;
		}
	else
		return CONSTLIT("Default");
	}

bool CDockScreen::EvalString (const CString &sString, ICCItem *pData, bool bPlain, ECodeChainEvents iEvent, CString *retsResult)

//	EvalString
//
//	Evaluates a string using CodeChain. A string that being with an equal sign
//	indicates a CodeChain expression.
//
//	Returns TRUE if successful. Otherwise, retsResult is an error.

	{
	CCodeChainCtx Ctx;
	Ctx.SetEvent(iEvent);
	Ctx.SetScreen(this);
	Ctx.SaveAndDefineSourceVar(m_pLocation);
	Ctx.SaveAndDefineDataVar(pData);

	return Ctx.RunEvalString(sString, bPlain, retsResult);
	}

CDockScreen::SDisplayControl *CDockScreen::FindDisplayControl (const CString &sID)

//	FindDisplayControl
//
//	Returns the display control with the given ID (or NULL if not found)

	{
	int i;

	for (i = 0; i < m_Controls.GetCount(); i++)
		{
		if (strEquals(sID, m_Controls[i].sID))
			return &m_Controls[i];
		}

	return NULL;
	}

ALERROR CDockScreen::FireOnScreenInit (CSpaceObject *pSource, ICCItem *pData, CString *retsError)

//	FireOnScreenInit
//
//	Fire screen OnScreenInit

	{
	//	We accept either OnScreenInit or OnInit

	CXMLElement *pOnInit = m_pDesc->GetContentElementByTag(ON_SCREEN_INIT_TAG);
	if (pOnInit == NULL)
		pOnInit = m_pDesc->GetContentElementByTag(ON_INIT_TAG);

	//	See if we have it

	if (pOnInit)
		{
		m_bInOnInit = true;

		CString sCode = pOnInit->GetContentText(0);

		//	Execute

		CCodeChainCtx Ctx;
		Ctx.SetScreen(this);
		Ctx.SaveAndDefineSourceVar(pSource);
		Ctx.SaveAndDefineDataVar(pData);

		ICCItem *pExp = Ctx.Link(sCode, 0, NULL);

		ICCItem *pResult = Ctx.Run(pExp);	//	LATER:Event
		Ctx.Discard(pExp);

		if (pResult->IsError())
			{
			*retsError = pResult->GetStringValue();

			Ctx.Discard(pResult);
			m_bInOnInit = false;
			return ERR_FAIL;
			}

		Ctx.Discard(pResult);
		m_bInOnInit = false;
		}

	m_bFirstOnInit = false;

	return NOERROR;
	}

void CDockScreen::HandleChar (char chChar)

//	HandleChar
//
//	Handle char events

	{
	m_CurrentPane.HandleChar(chChar);
	}

void CDockScreen::HandleKeyDown (int iVirtKey)

//	HandleKeyDown
//
//	Handle key down events

	{
	//	First see if the display will handle it

	IDockScreenDisplay::EResults iResult = m_pDisplay->HandleKeyDown(iVirtKey);

	switch (iResult)
		{
		//	If handled, then we're done

		case IDockScreenDisplay::resultHandled:
			return;

		//	If we need to reshow the pane, do it.

		case IDockScreenDisplay::resultShowPane:
			m_CurrentPane.ExecuteShowPane(EvalInitialPane());
			return;
		}

	//	Otherwise, handle it ourselves

	m_CurrentPane.HandleKeyDown(iVirtKey);
	}

ALERROR CDockScreen::InitCodeChain (CTranscendenceWnd *pTrans, CSpaceObject *pStation)

//	InitCodeChain
//
//	Initializes CodeChain language
//	LATER: We should define variables inside of Eval...

	{
	CCodeChain &CC = g_pUniverse->GetCC();

	//	Define some globals

	CC.DefineGlobalInteger(CONSTLIT("gSource"), (int)pStation);
	CC.DefineGlobalInteger(CONSTLIT("gScreen"), (int)this);

	return NOERROR;
	}

ALERROR CDockScreen::InitDisplay (CXMLElement *pDisplayDesc, AGScreen *pScreen, const RECT &rcScreen)

//	InitDisplay
//
//	Initializes display controls

	{
	int i;
	CCodeChain &CC = g_pUniverse->GetCC();

	ASSERT(m_Controls.GetCount() == 0);

	//	Init

	m_pDisplayInitialize = NULL;
	if (!pDisplayDesc->FindAttributeBool(ANIMATE_ATTRIB, &m_bDisplayAnimate))
		m_bDisplayAnimate = true;

	//	Allocate the controls

	int iControlCount = pDisplayDesc->GetContentElementCount();
	if (iControlCount == 0)
		return NOERROR;

	//	Compute the canvas rect for the controls (relative to pScreen)

	RECT rcCanvas;
	rcCanvas.left = rcScreen.left;
	rcCanvas.top = (RectHeight(rcScreen) - g_cyBackground) / 2;
	rcCanvas.right = rcCanvas.left + DESC_PANE_X;
	rcCanvas.bottom = rcCanvas.top + g_cyBackground;

	//	Create each control

	for (i = 0; i < iControlCount; i++)
		{
		CXMLElement *pControlDesc = pDisplayDesc->GetContentElement(i);

		//	If this is the initialize tag, remember it

		if (strEquals(pControlDesc->GetTag(), INITIALIZE_TAG)
				|| strEquals(pControlDesc->GetTag(), ON_DISPLAY_INIT_TAG))
			m_pDisplayInitialize = pControlDesc;

		//	Otherwise, Add the control

		else
			AddDisplayControl(pControlDesc, pScreen, NULL, rcCanvas);
		}

	return NOERROR;
	}

void CDockScreen::InitDisplayControlRect (CXMLElement *pDesc, const RECT &rcFrame, RECT *retrcRect)

//	InitDisplayControlRect
//
//	Initializes the RECT for a display control

	{
	RECT rcRect;

	rcRect.left = pDesc->GetAttributeInteger(LEFT_ATTRIB);
	rcRect.top = pDesc->GetAttributeInteger(TOP_ATTRIB);
	rcRect.right = pDesc->GetAttributeInteger(RIGHT_ATTRIB);
	rcRect.bottom = pDesc->GetAttributeInteger(BOTTOM_ATTRIB);

	int xCenter;
	bool bXCenter = pDesc->FindAttributeInteger(CENTER_ATTRIB, &xCenter);

	int yCenter;
	bool bYCenter = pDesc->FindAttributeInteger(VCENTER_ATTRIB, &yCenter);

	if (rcRect.right < 0)
		rcRect.right = RectWidth(rcFrame) + rcRect.right;

	if (rcRect.bottom < 0)
		rcRect.bottom = RectHeight(rcFrame) + rcRect.bottom;

	int cxWidth = pDesc->GetAttributeInteger(WIDTH_ATTRIB);
	if (cxWidth)
		{
		if (bXCenter)
			{
			rcRect.left = xCenter + ((RectWidth(rcFrame) - cxWidth) / 2);
			rcRect.right = rcRect.left + cxWidth;
			}
		else if (rcRect.right == 0)
			rcRect.right = rcRect.left + cxWidth;
		else if (rcRect.left == 0)
			rcRect.left = rcRect.right - cxWidth;
		}
	else
		{
		if (rcRect.right < rcRect.left)
			rcRect.right = RectWidth(rcFrame);
		}

	int cyHeight = pDesc->GetAttributeInteger(HEIGHT_ATTRIB);
	if (cyHeight)
		{
		if (bYCenter)
			{
			rcRect.top = yCenter + ((RectHeight(rcFrame) - cyHeight) / 2);
			rcRect.bottom = rcRect.top + cyHeight;
			}
		else if (rcRect.bottom == 0)
			rcRect.bottom = rcRect.top + cyHeight;
		else if (rcRect.top == 0)
			rcRect.top = rcRect.bottom - cyHeight;
		}
	else
		{
		if (rcRect.bottom < rcRect.top)
			rcRect.bottom = RectHeight(rcFrame);
		}

	//	If we're still 0-size, then we take up the whole frame

	if (rcRect.left == 0 && rcRect.top == 0
			&& rcRect.top == 0 && rcRect.bottom == 0)
		{
		rcRect.right = RectWidth(rcFrame);
		rcRect.bottom = RectHeight(rcFrame);
		}

	//	Otherwise, make sure we don't exceed the canvas

	else
		{
		rcRect.left = Min(RectWidth(rcFrame), Max(0, (int)rcRect.left));
		rcRect.right = Min(RectWidth(rcFrame), (int)Max(rcRect.left, rcRect.right));
		rcRect.top = Min(RectHeight(rcFrame), Max(0, (int)rcRect.top));
		rcRect.bottom = Min(RectHeight(rcFrame), (int)Max(rcRect.top, rcRect.bottom));
		}

	//	RECT is in absolute coordinates

	rcRect.left += rcFrame.left;
	rcRect.right += rcFrame.left;
	rcRect.top += rcFrame.top;
	rcRect.bottom += rcFrame.top;

	//	Done

	*retrcRect = rcRect;
	}

ALERROR CDockScreen::InitScreen (HWND hWnd, 
								 RECT &rcRect, 
								 const SDockFrame &Frame,
								 CExtension *pExtension,
								 CXMLElement *pDesc, 
								 const CString &sPane,
								 ICCItem *pData,
								 CString *retsPane,
								 AGScreen **retpScreen)

//	InitScreen
//
//	Initializes the docking screen. Returns an AGScreen object
//	that has been initialized appropriately.

	{
	ALERROR error;

	//	Make sure we clean up first

	CleanUpScreen();
	m_pFonts = &g_pTrans->GetFonts();

	//	Init some variables

	m_pLocation = Frame.pLocation;
	m_pRoot = Frame.pRoot;
	m_sScreen = Frame.sScreen;
	m_pData = pData;
	m_pPlayer = g_pTrans->GetPlayer();
	m_pExtension = pExtension;
	m_pDesc = pDesc;

	//	Initialize CodeChain processor

	if (error = InitCodeChain(g_pTrans, m_pLocation))
		return error;

	//	Call OnScreenInit

	CString sError;
	if (error = FireOnScreenInit(m_pLocation, pData, &sError))
		{
		kernelDebugLogMessage(sError);
		//	We do not fail because otherwise the screen would be invalid.
		}

	//	If we've already got a screen set up then we don't need to
	//	continue (OnScreenInit has navigated to a different screen).

	if (m_pScreen)
		{
		if (retsPane)
			*retsPane = NULL_STR;

		return NOERROR;
		}

	//	Create a new screen

	m_pScreen = new AGScreen(hWnd, rcRect);
	m_pScreen->SetController(this);
	m_pScreen->SetBackgroundColor(BAR_COLOR);

	RECT rcBackground;
	int cxBackground = Min(MAX_BACKGROUND_WIDTH, RectWidth(rcRect));
	int cyBackground = RectHeight(rcRect);
	rcBackground.left = (RectWidth(rcRect) - cxBackground) / 2;
	rcBackground.top = 0;
	rcBackground.right = rcBackground.left + cxBackground;
	rcBackground.bottom = rcBackground.top + cyBackground;

	RECT rcScreen;
	int cxScreen = Min(MAX_SCREEN_WIDTH, RectWidth(rcRect));
	int cyScreen = RectHeight(rcRect);
	rcScreen.left = (RectWidth(rcRect) - cxScreen) / 2;
	rcScreen.top = 0;
	rcScreen.right = rcScreen.left + cxScreen;
	rcScreen.bottom = rcScreen.top + cyScreen;

	//	Creates the title area

	if (error = CreateTitleArea(m_pDesc, m_pScreen, rcBackground, rcScreen))
		return error;

	//	Get the list of panes for this screen

	m_pPanes = m_pDesc->GetContentElementByTag(PANES_TAG);

	//	Create the main display object based on the type parameter.

	CString sType = m_pDesc->GetAttribute(TYPE_ATTRIB);

	if (strEquals(sType, SCREEN_TYPE_ITEM_PICKER))
		m_pDisplay = new CDockScreenItemList;

	else if (strEquals(sType, SCREEN_TYPE_CUSTOM_PICKER))
		m_pDisplay = new CDockScreenCustomList;

	else if (strEquals(sType, SCREEN_TYPE_CUSTOM_ITEM_PICKER))
		m_pDisplay = new CDockScreenCustomItemList;

	else if (strEquals(sType, SCREEN_TYPE_ARMOR_SELECTOR))
		m_pDisplay = new CDockScreenSelector(CGSelectorArea::configArmor);

	else
		m_pDisplay = new CDockScreenNullDisplay;

	//	Initialize

	IDockScreenDisplay::SInitCtx DisplayCtx;
	DisplayCtx.pPlayer = m_pPlayer;
	DisplayCtx.dwFirstID = DISPLAY_ID;
	DisplayCtx.pData = pData;
	DisplayCtx.pDesc = m_pDesc;
	DisplayCtx.pDockScreen = this;
	DisplayCtx.pVI = &g_pHI->GetVisuals();
	DisplayCtx.pFontTable = m_pFonts;
	DisplayCtx.pLocation = m_pLocation;
	DisplayCtx.pScreen = m_pScreen;

	DisplayCtx.rcRect = rcScreen;
	DisplayCtx.rcRect.top = (RectHeight(rcScreen) - g_cyDockScreen) / 2;
	DisplayCtx.rcRect.right = DisplayCtx.rcRect.left + DESC_PANE_X;
	DisplayCtx.rcRect.bottom = DisplayCtx.rcRect.top + g_cyDockScreen;

	if (error = m_pDisplay->Init(DisplayCtx, &sError))
		{
		SetDescription(sError);
		kernelDebugLogMessage(sError);

		//	Continue
		}

	//	If we have a display element, then load the display controls

	CXMLElement *pDisplay = m_pDesc->GetContentElementByTag(DISPLAY_TAG);
	if (pDisplay)
		{
		if (error = InitDisplay(pDisplay, m_pScreen, rcScreen))
			return error;
		}

	//	Create the background area. We do this after the display init because we
	//	may need to refer to it to come up with a suitable background.

	if (error = CreateBackgroundArea(m_pDesc, m_pScreen, rcBackground, rcScreen))
		return error;

	//	Cache the screen's OnUpdate

	CXMLElement *pOnUpdate = m_pDesc->GetContentElementByTag(ON_SCREEN_UPDATE_TAG);
	if (pOnUpdate)
		{
		CCodeChainCtx Ctx;
		m_pOnScreenUpdate = Ctx.Link(pOnUpdate->GetContentText(0), 0, NULL);
		if (m_pOnScreenUpdate->IsError())
			{
			kernelDebugLogMessage("Unable to parse OnScreenUpdate: %s.", m_pOnScreenUpdate->GetStringValue());
			Ctx.Discard(m_pOnScreenUpdate);
			m_pOnScreenUpdate = NULL;
			}
		}

	//	Show the pane

	m_rcPane.left = rcScreen.right - g_cxActionsRegion;
	m_rcPane.top = (RectHeight(rcScreen) - g_cyDockScreen) / 2;
	m_rcPane.right = rcScreen.right - 8;
	m_rcPane.bottom = m_rcPane.top + g_cyBackground;

	if (!sPane.IsBlank())
		{
		ShowPane(sPane);
		if (retsPane)
			*retsPane = sPane;
		}
	else
		{
		CString sPane = EvalInitialPane(m_pLocation, pData);

		//	If evaluation fails due to an error, then show default

		if (sPane.IsBlank())
			sPane = CONSTLIT("Default");

		ShowPane(sPane);
		if (retsPane)
			*retsPane = sPane;
		}

	//	Done

	*retpScreen = m_pScreen;

	return NOERROR;
	}

ALERROR CDockScreen::ReportError (const CString &sError)

//	ReportError
//
//	Reports an error while evaluating some function in the dock screen.

	{
	CString sNewError = strPatternSubst(CONSTLIT("%08x%s: %s"),
				(m_pRoot ? m_pRoot->GetUNID() : 0),
				(m_sScreen.IsBlank() ? NULL_STR : strPatternSubst(CONSTLIT("/%s"), m_sScreen)),
				sError);

	SetDescription(sNewError);
	kernelDebugLogMessage(sNewError);

	return ERR_FAIL;
	}

void CDockScreen::ResetList (CSpaceObject *pLocation)

//	ResetList
//
//	Resets the display list

	{
	if (m_pDisplay->ResetList(pLocation) == IDockScreenDisplay::resultShowPane)
		m_CurrentPane.ExecuteShowPane(EvalInitialPane());
	}

void CDockScreen::ShowDisplay (bool bAnimateOnly)

//	ShowDisplay
//
//	Updates the controls on the display

	{
	int i;
	CCodeChain &CC = g_pUniverse->GetCC();

	//	Run initialize

	if (m_pDisplayInitialize)
		{
		CString sCode = m_pDisplayInitialize->GetContentText(0);
		CString sError;
		if (!EvalString(sCode, m_pData, true, eventNone, &sError))
			ReportError(strPatternSubst(CONSTLIT("Error evaluating <OnDisplayInit>: %s"), sError));
		}

	//	Set controls

	for (i = 0; i < m_Controls.GetCount(); i++)
		{
		//	If we're animating, skip controls that don't need animation

		if (bAnimateOnly && !m_Controls[i].bAnimate)
			continue;

		//	Update control based on type

		switch (m_Controls[i].iType)
			{
			case ctrlImage:
				{
				CGImageArea *pControl = (CGImageArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx;
					Ctx.SetScreen(this);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogMessage(pResult->GetStringValue());
							}
						Ctx.Discard(pResult);
						break;
						}

					//	The result is the image descriptor

					CG16bitImage *pImage;
					RECT rcImage;
					GetImageDescFromList(CC, pResult, &pImage, &rcImage);
					if (pImage)
						pControl->SetImage(pImage, rcImage);

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlCanvas:
				{
				CGDrawArea *pControl = (CGDrawArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx;
					Ctx.SetScreen(this);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);
					CG16bitImage *pCanvas = &pControl->GetCanvas();
					Ctx.SetCanvas(pCanvas);

					pCanvas->Fill(0, 0, pCanvas->GetWidth(), pCanvas->GetHeight(), 0);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogMessage(pResult->GetStringValue());
							}

						Ctx.Discard(pResult);
						break;
						}

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlNeurohack:
				{
				CGNeurohackArea *pControl = (CGNeurohackArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx;
					Ctx.SetScreen(this);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogMessage(pResult->GetStringValue());
							}

						Ctx.Discard(pResult);
						break;
						}

					//	The result is the data descriptor in this order:
					//
					//	Willpower
					//	Damage

					int iWillpower = (pResult->GetCount() > 0 ? pResult->GetElement(0)->GetIntegerValue() : 0);
					int iDamage = (pResult->GetCount() > 1 ? pResult->GetElement(1)->GetIntegerValue() : 0);
					pControl->SetData(iWillpower, iDamage);

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlText:
				{
				CGTextArea *pControl = (CGTextArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx;
					Ctx.SetScreen(this);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	The result is the text for the control

					pControl->SetText(pResult->GetStringValue());

					//	If we have an error, report it as well

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogMessage(pResult->GetStringValue());
							}
						}

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}
			}
		}
	}

ALERROR CDockScreen::SetDisplayText (const CString &sID, const CString &sText)

//	SetDisplayText
//
//	Sets the text for a display control

	{
	SDisplayControl *pControl = FindDisplayControl(sID);
	if (pControl == NULL || pControl->pArea == NULL)
		return ERR_FAIL;

	if (pControl->iType != ctrlText)
		return ERR_FAIL;

	CGTextArea *pTextControl = (CGTextArea *)pControl->pArea;
	pTextControl->SetText(g_pTrans->ComposePlayerNameString(sText));

	//	If we're explicitly setting the text, then we cannot animate

	pControl->bAnimate = false;

	return NOERROR;
	}

void CDockScreen::SetListCursor (int iCursor)

//	SetListCursor
//
//	Sets the list cursor

	{
	if (m_pDisplay->SetListCursor(iCursor) == IDockScreenDisplay::resultShowPane)
		m_CurrentPane.ExecuteShowPane(EvalInitialPane());
	}

void CDockScreen::SetListFilter (const CItemCriteria &Filter)

//	SetListFilter
//
//	Filters the list given the criteria

	{
	if (m_pDisplay->SetListFilter(Filter) == IDockScreenDisplay::resultShowPane)
		m_CurrentPane.ExecuteShowPane(EvalInitialPane());
	}

void CDockScreen::ShowPane (const CString &sName)

//	ShowPane
//
//	Shows the pane of the given name

	{
	const CVisualPalette &VI = g_pHI->GetVisuals();

#ifdef DEBUG_STRING_LEAKS
	CString::DebugMark();
#endif

	//	If sName is blank, then it is likely due to EvalInitialPane failing

	if (sName.IsBlank())
		return;

	//	Find the pane named

	CXMLElement *pNewPane = m_pPanes->GetContentElementByTag(sName);
	if (pNewPane == NULL)
		{
		CString sError = strPatternSubst(CONSTLIT("Unable to find pane: %s"), sName);
		SetDescription(sError);
		kernelDebugLogMessage(sError);
		return;
		}

	//	Initialize the pane based on the pane descriptor

	m_CurrentPane.InitPane(this, pNewPane, m_rcPane);

	//	Update screen
	//	Show the currently selected item

	m_pDisplay->ShowPane(pNewPane->GetAttributeBool(NO_LIST_NAVIGATION_ATTRIB));

	//	Update the display

	if (m_Controls.GetCount() > 0)
		ShowDisplay();

	UpdateCredits();

#ifdef DEBUG_STRING_LEAKS
	CString::DebugOutputLeakedStrings();
#endif
	}

void CDockScreen::SelectNextItem (bool *retbMore)

//	SelectNextItem
//
//	Selects the next item in the list

	{
	bool bMore = m_pDisplay->SelectNextItem();
	if (retbMore)
		*retbMore = bMore;
	}

void CDockScreen::SelectPrevItem (bool *retbMore)

//	SelectPrevItem
//
//	Selects the previous item in the list

	{
	bool bMore = m_pDisplay->SelectPrevItem();
	if (retbMore)
		*retbMore = bMore;
	}

bool CDockScreen::Translate (const CString &sTextID, ICCItem *pData, ICCItem **retpResult)

//	Translate
//
//	Translate text

	{
	CDesignType *pRoot = GetResolvedRoot();
	if (pRoot == NULL)
		return false;

	return pRoot->Translate(m_pLocation, sTextID, pData, retpResult);
	}

void CDockScreen::Update (int iTick)

//	Update
//
//	Updates the display

	{
	DEBUG_TRY

	if (m_pScreen)
		m_pScreen->Update();

	if (m_bDisplayAnimate && (iTick % 10) == 0)
		{
		ShowDisplay(true);
		}

	//	Call OnScreenUpdate every 15 ticks

	if (m_pOnScreenUpdate
			&& ((iTick % ON_SCREEN_UPDATE_CYCLE) == 0))
		{
		//	Add a reference to m_pOnScreenUpdate because we might
		//	reinitialize the screen inside the call.

		ICCItem *pCode = m_pOnScreenUpdate->Reference();

		//	Execute

		CCodeChainCtx Ctx;
		Ctx.SetScreen(this);
		Ctx.SaveAndDefineSourceVar(m_pLocation);
		Ctx.SaveAndDefineDataVar(m_pData);

		ICCItem *pResult = Ctx.Run(pCode);

		if (pResult->IsError())
			kernelDebugLogMessage(CONSTLIT("<OnScreenUpdate>: %s"), pResult->GetStringValue());

		Ctx.Discard(pResult);
		Ctx.Discard(pCode);
		}

	DEBUG_CATCH
	}

void CDockScreen::UpdateCredits (void)

//	UpdateCredits
//
//	Updates the display of credits

	{
	//	Money

	CEconomyType *pEconomy = m_pLocation->GetDefaultEconomy();
	m_pCredits->SetText(strPatternSubst(CONSTLIT("%s: %d"), 
			strCapitalize(pEconomy->GetCurrencyNamePlural()),
			(int)m_pPlayer->GetCredits(pEconomy->GetUNID())
			));

	//	Cargo space

	m_pCargoSpace->SetText(strPatternSubst("%d ton%p", m_pPlayer->GetCargoSpace()));
	}
