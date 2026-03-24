#include "pch.h"
#include "eclipse.h"
#include "input.h"
#include "GameApp.h"
#include "script.h"
#include "attract.h"
#include "taskmanager_interface.h"

AttractMode::AttractMode()
:	m_running(false)
{
	strncpy(m_userProfile, "none", sizeof(m_userProfile));
	m_userProfile[sizeof(m_userProfile) - 1] = '\0';
}

void AttractMode::Advance()
{
	if( !m_running )
	{
		if( g_inputManager->controlEvent( ControlStartAttractMode ) ) // change to a timer for user idleness
		{
			StartAttractMode();
		}
	}
	else
	{
		if( g_inputManager->controlEvent( ControlStopAttractMode ) ||
			!g_context->m_script->IsRunningScript() ) // change to check for any user activity
		{
			EndAttractMode();
		}
	}
}

void AttractMode::StartAttractMode()
{
	strncpy( m_userProfile, g_context->m_userProfileName, sizeof(m_userProfile) );
	m_userProfile[sizeof(m_userProfile) - 1] = '\0';
	g_gameApp->SetProfileName( "AttractMode" );
	g_context->m_taskManagerInterface->m_lockTaskManager = true;
	if( g_gameApp->LoadProfile() )
	{
		g_context->m_script->RunScript("attractmode.txt");
		m_running = true;
	}

	LList<EclWindow *> *windows = EclGetWindows();
    while (windows->Size() > 0) {
        EclWindow *w = windows->GetData(0);
        EclRemoveWindow(w->m_name);
    }


}

void AttractMode::EndAttractMode()
{
    g_context->m_script->m_waitForCamera = false;
	g_context->m_script->RunScript("endattract.txt");
	g_gameApp->SetProfileName( m_userProfile );
	g_gameApp->LoadProfile();

    g_context->m_taskManagerInterface->m_lockTaskManager = false;

	m_running = false;
	strncpy(m_userProfile, "none", sizeof(m_userProfile));
	m_userProfile[sizeof(m_userProfile) - 1] = '\0';
}
