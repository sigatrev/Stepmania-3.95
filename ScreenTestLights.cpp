#include "global.h"
#include "ScreenTestLights.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "LightsManager.h"
#include "Game.h"
#include "ScreenDimensions.h"


REGISTER_SCREEN_CLASS( ScreenTestLights );
ScreenTestLights::ScreenTestLights( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenTestLights::ScreenTestLights()" );

	LIGHTSMAN->SetLightsMode( LIGHTSMODE_TEST_AUTO_CYCLE );
}

void ScreenTestLights::Init()
{
	ScreenWithMenuElements::Init();

	m_textInputs.SetName( "Text" );
	m_textInputs.LoadFromFont( THEME->GetPathF("Common","normal") );
	m_textInputs.SetText( "" );
	SET_XY_AND_ON_COMMAND( m_textInputs );
	this->AddChild( &m_textInputs );

	this->SortByDrawOrder();
}



ScreenTestLights::~ScreenTestLights()
{
	LOG->Trace( "ScreenTestLights::~ScreenTestLights()" );
}


void ScreenTestLights::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );


	if( m_timerBackToAutoCycle.Ago() > 20 )
	{
		m_timerBackToAutoCycle.Touch();
		LIGHTSMAN->SetLightsMode( LIGHTSMODE_TEST_AUTO_CYCLE );
	}


	CabinetLight cl = LIGHTSMAN->GetFirstLitCabinetLight();
	GameController gc;
	GameButton gb;
	LIGHTSMAN->GetFirstLitGameButtonLight( gc, gb );

	CString s;

	switch( LIGHTSMAN->GetLightsMode() )
	{
	case LIGHTSMODE_TEST_AUTO_CYCLE:
		s += "Auto Cycle\n";
		break;
	case LIGHTSMODE_TEST_MANUAL_CYCLE:
		s += "Manual Cycle\n";
		break;
	}

	if( cl == LIGHT_INVALID )
		s += "cabinet light: -----\n";
	else
		s += ssprintf( "cabinet light: %d %s\n", cl, CabinetLightToString(cl).c_str() );

	if( gc == GAME_CONTROLLER_INVALID )
	{
		s += ssprintf( "controller light: -----\n" );
	}
	else
	{
		CString sGameButton = GAMESTATE->GetCurrentGame()->m_szButtonNames[gb];
		s += ssprintf( "controller light: P%d %d %s\n", gc+1, gb, sGameButton.c_str() );
	}

	m_textInputs.SetText( s );
}


void ScreenTestLights::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenTestLights::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenTestLights::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		LIGHTSMAN->SetLightsMode( LIGHTSMODE_MENU );
		break;
	}
}

void ScreenTestLights::MenuLeft( PlayerNumber pn )
{
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_TEST_MANUAL_CYCLE );
	if( pn == PLAYER_1 )
		LIGHTSMAN->PrevTestCabinetLight();
	else
		LIGHTSMAN->PrevTestGameButtonLight();
	m_timerBackToAutoCycle.Touch();
}

void ScreenTestLights::MenuRight( PlayerNumber pn )
{
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_TEST_MANUAL_CYCLE );
	if( pn == PLAYER_1 )
		LIGHTSMAN->NextTestCabinetLight();
	else
		LIGHTSMAN->NextTestGameButtonLight();
	m_timerBackToAutoCycle.Touch();
}

void ScreenTestLights::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}

void ScreenTestLights::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		SCREENMAN->PlayStartSound();
		OFF_COMMAND( m_textInputs );
		StartTransitioning( SM_GoToPrevScreen );		
	}
}

/*
 * (c) 2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
