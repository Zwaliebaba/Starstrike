
#pragma once

#include "darwinia_window.h"


class NetworkWindow : public DarwiniaWindow
{
public:
    NetworkWindow( char *name );

    void Render( bool hasFocus );
};

