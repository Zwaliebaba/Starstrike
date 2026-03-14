
#pragma once

#include "darwinia_window.h"


class NetworkWindow : public DarwiniaWindow
{
public:
    NetworkWindow( const char *name );

    void Render( bool hasFocus );
};

