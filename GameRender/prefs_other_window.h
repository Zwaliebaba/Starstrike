
#pragma once

#include "darwinia_window.h"


class PrefsOtherWindow : public DarwiniaWindow
{
public:
    int     m_helpEnabled;
	int		m_controlHelpEnabled;
    int     m_bootLoader;
    int     m_christmas;
    int     m_language;
	int		m_difficulty;
	int		m_largeMenus;
    int     m_automaticCamera;

    LList   <char *> m_languages;

public:
    PrefsOtherWindow();
    ~PrefsOtherWindow();

    void Create();
    void Render( bool _hasFocus );

    void ListAvailableLanguages();
};

#include "prefs_keys.h"

