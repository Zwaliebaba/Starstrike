#include "pch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"

#include "resource.h"
#include "shape.h"
#include "text_stream_readers.h"
#include "debug_render.h"
#include "file_writer.h"
#include "text_renderer.h"
#include "language_table.h"

#include "blueprintstore.h"
#include "darwinian.h"

#include "app.h"
#include "location.h"
#include "renderer.h"
#include "camera.h"
#include "entity_grid.h"
#include "main.h"
#include "team.h"
#include "global_world.h"


BlueprintBuilding::BlueprintBuilding()
:   Building(),
    m_buildingLink(-1),
    m_infected(0.0f),
    m_segment(0),
    m_marker(NULL)
{
    m_vel.Zero();
}


void BlueprintBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_marker = m_shape->m_rootFragment->LookupMarker( "MarkerBlueprint" );
    DarwiniaDebugAssert( m_marker );

    BlueprintBuilding *blueprintBuilding = (BlueprintBuilding *) _template;

    m_buildingLink = blueprintBuilding->m_buildingLink;

    if( m_id.GetTeamId() == 1 ) m_infected = 100.0f;
    else                        m_infected = 0.0f;
}


bool BlueprintBuilding::Advance()
{
    BlueprintBuilding *blueprintBuilding = (BlueprintBuilding *) g_app->m_location->GetBuilding( m_buildingLink );
    if( blueprintBuilding )
    {
        if( m_infected > 80.0f ) blueprintBuilding->SendBlueprint( m_segment, true );
        if( m_infected < 20.0f ) blueprintBuilding->SendBlueprint( m_segment, false );
    }

    return Building::Advance();
}


Matrix34 BlueprintBuilding::GetMarker( float _predictionTime )
{
    LegacyVector3 pos = m_pos + m_vel * _predictionTime;
    Matrix34 mat( m_front, g_upVector, pos );

    if( m_marker )
    {
        Matrix34 markerMat = m_marker->GetWorldMatrix(mat);
        return markerMat;
    }
    else
    {
        return mat;
    }
}


bool BlueprintBuilding::IsInView()
{
    Building *link = g_app->m_location->GetBuilding( m_buildingLink );

    if( link )
    {
        LegacyVector3 midPoint = ( link->m_centrePos + m_centrePos ) / 2.0f;
        float radius = ( link->m_centrePos - m_centrePos ).Mag();
        radius += m_radius;
        return( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) );
    }
    else
    {
        return Building::IsInView();
    }
}


void BlueprintBuilding::Render( float _predictionTime )
{
    LegacyVector3 pos = m_pos+m_vel*_predictionTime;
	Matrix34 mat(m_front, g_upVector, pos);
    m_shape->Render(_predictionTime, mat);
}


void BlueprintBuilding::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    BlueprintBuilding *link = (BlueprintBuilding *) g_app->m_location->GetBuilding( m_buildingLink );
    if( link )
    {
        float infected = m_infected / 100.0f;
        float linkInfected = link->m_infected / 100.0f;
        float ourTime = g_gameTime + m_id.GetUniqueId() + m_id.GetIndex();
        if( fabs( infected - linkInfected ) < 0.01f )
        {
            g_imRenderer->Color4f( infected, 0.7f-infected*0.7f, 0.0f, 0.1f+fabs(sinf(ourTime))*0.2f );
        }
        else
        {
            g_imRenderer->Color4f( infected, 0.7f-infected*0.7f, 0.0f, 0.5f+fabs(sinf(ourTime))*0.5f );
        }

        LegacyVector3 ourPos = GetMarker(_predictionTime).pos;
        LegacyVector3 theirPos = link->GetMarker(_predictionTime).pos;

        LegacyVector3 rightAngle = ( g_app->m_camera->GetPos() - ourPos ) ^ ( theirPos - ourPos );
        rightAngle.SetLength( 20.0f );

        g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
        g_imRenderer->BindTexture(g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
        g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

        g_imRenderer->Begin(PRIM_QUADS);
            g_imRenderer->TexCoord2i(0,0);      g_imRenderer->Vertex3fv( (ourPos - rightAngle).GetData() );
            g_imRenderer->TexCoord2i(0,1);      g_imRenderer->Vertex3fv( (ourPos + rightAngle).GetData() );
            g_imRenderer->TexCoord2i(1,1);      g_imRenderer->Vertex3fv( (theirPos + rightAngle).GetData() );
            g_imRenderer->TexCoord2i(1,0);      g_imRenderer->Vertex3fv( (theirPos - rightAngle).GetData() );
        g_imRenderer->End();


        g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
        g_imRenderer->UnbindTexture();
    }

    //g_editorFont.DrawText3DCentre( m_pos+LegacyVector3(0,50,0), 10.0f, "%d Infected %2.2f", m_segment, m_infected );
}


void BlueprintBuilding::SendBlueprint( int _segment, bool _infected )
{
    m_segment = _segment;

    if( _infected ) m_infected += SERVER_ADVANCE_PERIOD * 10.0f;
    else            m_infected -= SERVER_ADVANCE_PERIOD * 10.0f;

    m_infected = max( m_infected, 0.0f );
    m_infected = min( m_infected, 100.0f );
}


void BlueprintBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_buildingLink = atoi( _in->GetNextToken() );
}


void BlueprintBuilding::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_buildingLink );
}


int BlueprintBuilding::GetBuildingLink()
{
    return m_buildingLink;
}


void BlueprintBuilding::SetBuildingLink( int _buildingId )
{
    m_buildingLink = _buildingId;
}


// ============================================================================


BlueprintStore::BlueprintStore()
:   BlueprintBuilding()
{
    m_type = Building::TypeBlueprintStore;

    SetShape( g_app->m_resource->GetShape( "blueprintstore.shp" ) );
}


char *BlueprintStore::GetObjectiveCounter()
{
    static char result[256];

    float totalInfection = 0;
    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
        totalInfection += m_segments[i];

    sprintf( result, "%s %d%%", LANGUAGEPHRASE( "objective_totalinfection" ),
                                int( 100.0f * totalInfection / float(BLUEPRINTSTORE_NUMSEGMENTS * 100.0f) ) );

    return result;
}


void BlueprintStore::Initialise( Building *_template )
{
    BlueprintBuilding::Initialise( _template );

    if( m_id.GetTeamId() == 1 )
    {
        for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
        {
            m_segments[i] = 100.0f;
        }
    }
    else
    {
        for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
        {
            m_segments[i] = 0.0f;
        }
    }
}


void BlueprintStore::SendBlueprint( int _segment, bool _infected )
{
    float oldValue = m_segments[_segment];

    if( _infected ) oldValue += SERVER_ADVANCE_PERIOD * 1.0f;
    else            oldValue -= SERVER_ADVANCE_PERIOD * 1.0f;

    oldValue = max( oldValue, 0.0f );
    oldValue = min( oldValue, 100.0f );

    m_segments[_segment] = oldValue;
}


bool BlueprintStore::Advance()
{
    int fullyInfected = 0;
    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
    {
        if( m_segments[i] == 100.0f )
        {
            fullyInfected++;
        }
    }
    int clean = BLUEPRINTSTORE_NUMSEGMENTS - fullyInfected;


    //
    // Spread our existing infection

    if( clean > 0 )
    {
        float infectionChange = (fullyInfected * 0.9f) / (float) clean;

        for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
        {
            if( m_segments[i] < 100.0f )
            {
                float oldValue = m_segments[i];
                oldValue += SERVER_ADVANCE_PERIOD * infectionChange;
                oldValue = max( oldValue, 0.0f );
                oldValue = min( oldValue, 100.0f );
                m_segments[i] = oldValue;
            }
        }
    }


    //
    // Are we clean?

    bool totallyClean = true;
    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
    {
        if( m_segments[i] > 0.0f )
        {
            totallyClean = false;
            break;
        }
    }

    if( totallyClean )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_online = true;
    }

    return BlueprintBuilding::Advance();
}


int BlueprintStore::GetNumInfected()
{
    int result = 0;
    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
    {
        if( !(m_segments[i] < 100.0f) )
        {
            ++result;
        }
    }
    return result;
}


int BlueprintStore::GetNumClean()
{
    int result = 0;
    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
    {
        if( !(m_segments[i] > 0.0f ) )
        {
            ++result;
        }
    }
    return result;
}


void BlueprintStore::GetDisplay( LegacyVector3 &_pos, LegacyVector3 &_right, LegacyVector3 &_up, float &_size )
{
    _size = 50.0f;

    LegacyVector3 front = m_front;
    front.RotateAroundY( sinf(g_gameTime) * 0.3f );

    LegacyVector3 up = g_upVector;
    up.RotateAround( m_front * cosf(g_gameTime) * 0.1f );

    _pos = m_pos + up * 50.0f - front * _size;
    _right = front;
    _up = up;
}


void BlueprintStore::Render( float _predictionTime )
{
    BlueprintBuilding::Render( _predictionTime );

//    for( int i = 0; i < BLUEPRINTSTORE_NUMSEGMENTS; ++i )
//    {
//        g_editorFont.DrawText3DCentre( m_pos+LegacyVector3(0,170+i*20,0), 20, "Segment %d : %2.2f", i, m_segments[i] );
//    }
}


/*
void BlueprintStore::RenderAlphas( float _predictionTime )
{
    BlueprintBuilding::RenderAlphas( _predictionTime );

    LegacyVector3 screenPos, screenRight, screenUp;
    float screenSize;
    GetDisplay( screenPos, screenRight, screenUp, screenSize );

    g_imRenderer->Color4f( 1.0f, 1.0f, 1.0f, 0.75f );
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_imRenderer->BindTexture(g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

    int numSteps = sqrt(BLUEPRINTSTORE_NUMSEGMENTS);

    for( int x = 0; x < numSteps; ++x )
    {
        for( int y = 0; y < numSteps; ++y )
        {
            float tx = (float) x / (float) numSteps;
            float ty = (float) y / (float) numSteps;
            float size = 1.0f / (float) numSteps;

            LegacyVector3 pos = screenPos + x * screenRight * screenSize
                                    + y * screenUp * screenSize;
            LegacyVector3 width = screenRight * screenSize;
            LegacyVector3 height = screenUp * screenSize;

            float infected = m_segments[y*numSteps+x] / 100.0f;
            g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );

            g_imRenderer->Begin(PRIM_QUADS);
                g_imRenderer->TexCoord2f(tx,ty);            g_imRenderer->Vertex3fv( pos.GetData() );
                g_imRenderer->TexCoord2f(tx+size,ty);       g_imRenderer->Vertex3fv( (pos+width).GetData() );
                g_imRenderer->TexCoord2f(tx+size,ty+size);  g_imRenderer->Vertex3fv( (pos+width+height).GetData() );
                g_imRenderer->TexCoord2f(tx,ty+size);       g_imRenderer->Vertex3fv( (pos+height).GetData() );
            g_imRenderer->End();

        }
    }

    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_imRenderer->UnbindTexture();
}*/


void BlueprintStore::RenderAlphas( float _predictionTime )
{
    BlueprintBuilding::RenderAlphas( _predictionTime );

    LegacyVector3 screenPos, screenRight, screenUp;
    float screenSize;
    GetDisplay( screenPos, screenRight, screenUp, screenSize );

    //
    // Render main darwinian

    g_imRenderer->BindTexture(g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);

    float texX = 0.0f;
    float texY = 0.0f;
    float texH = 1.0f;
    float texW = 1.0f;


    g_imRenderer->Begin(PRIM_QUADS);
        float infected = m_segments[0] / 100.0f;
        g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
        g_imRenderer->TexCoord2f(texX,texY);
        g_imRenderer->Vertex3fv( screenPos.GetData() );

        infected = m_segments[1] / 100.0f;
        g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
        g_imRenderer->TexCoord2f(texX+texW,texY);
        g_imRenderer->Vertex3fv( (screenPos + screenRight * screenSize * 2).GetData() );

        infected = m_segments[2] / 100.0f;
        g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
        g_imRenderer->TexCoord2f(texX+texW,texY+texH);
        g_imRenderer->Vertex3fv( (screenPos + screenRight * screenSize * 2 + screenUp * screenSize * 2).GetData() );

        infected = m_segments[3] / 100.0f;
        g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
        g_imRenderer->TexCoord2f(texX,texY+texH);
        g_imRenderer->Vertex3fv( (screenPos + screenUp * screenSize * 2).GetData() );
    g_imRenderer->End();

    //
    // Render lines for over effect

    g_imRenderer->BindTexture(g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

    texX = 0.0f;
    texW = 3.0f;
    texY = g_gameTime*0.01f;
    texH = 0.3f;

    for( int i = 0; i < 2; ++i )
    {
        g_imRenderer->Begin(PRIM_QUADS);
            float infected = m_segments[0] / 100.0f;
            g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
            g_imRenderer->TexCoord2f(texX,texY);
            g_imRenderer->Vertex3fv( screenPos.GetData() );

            infected = m_segments[1] / 100.0f;
            g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
            g_imRenderer->TexCoord2f(texX+texW,texY);
            g_imRenderer->Vertex3fv( (screenPos + screenRight * screenSize * 2).GetData() );

            infected = m_segments[2] / 100.0f;
            g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
            g_imRenderer->TexCoord2f(texX+texW,texY+texH);
            g_imRenderer->Vertex3fv( (screenPos + screenRight * screenSize * 2 + screenUp * screenSize * 2).GetData() );

            infected = m_segments[3] / 100.0f;
            g_imRenderer->Color4f( infected*0.8f, 0.8f-infected*0.8f, 0.0f, 1.0f );
            g_imRenderer->TexCoord2f(texX,texY+texH);
            g_imRenderer->Vertex3fv( (screenPos + screenUp * screenSize * 2).GetData() );
        g_imRenderer->End();

        texY *= 1.5f;
        texH = 0.1f;
    }


    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_imRenderer->UnbindTexture();
}


// ============================================================================


BlueprintConsole::BlueprintConsole()
:   BlueprintBuilding()
{
    m_type = Building::TypeBlueprintConsole;

    SetShape( g_app->m_resource->GetShape( "blueprintconsole.shp" ) );
}


void BlueprintConsole::Initialise( Building *_template )
{
    BlueprintBuilding::Initialise( _template );

    BlueprintConsole *console = (BlueprintConsole *) _template;
    m_segment = console->m_segment;
}


void BlueprintConsole::RecalculateOwnership()
{
    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS*sizeof(int));

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
            teamCount[id.GetTeamId()] ++;
        }
    }

    int winningTeam = -1;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > 2 &&
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 &&
                 teamCount[i] > 2 &&
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
    else
    {
        SetTeamId(winningTeam);
    }
}


void BlueprintConsole::Read( TextReader *_in, bool _dynamic )
{
    BlueprintBuilding::Read( _in, _dynamic );

    m_segment = atoi( _in->GetNextToken() );
}


void BlueprintConsole::Write( FileWriter *_out )
{
    BlueprintBuilding::Write( _out );

    _out->printf( "%-8d", m_segment );
}


bool BlueprintConsole::Advance()
{
    RecalculateOwnership();

    bool infected = ( m_id.GetTeamId() == 1 );
    bool clean = ( m_id.GetTeamId() == 0 );

    if( infected )  SendBlueprint( m_segment, true );
    if( clean )     SendBlueprint( m_segment, false );

    return BlueprintBuilding::Advance();
}


void BlueprintConsole::Render( float _predictionTime )
{
    BlueprintBuilding::Render( _predictionTime );

    Matrix34 mat( m_front, g_upVector, m_pos );
    m_shape->Render( 0.0f, mat );
}


void BlueprintConsole::RenderPorts()
{
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        LegacyVector3 portPos;
        LegacyVector3 portFront;
        GetPortPosition( i, portPos, portFront );

        LegacyVector3 portUp = g_upVector;
        Matrix34 mat( portFront, portUp, portPos );

        //
        // Render the status light

        float size = 6.0f;
        LegacyVector3 camR = g_app->m_camera->GetRight() * size;
        LegacyVector3 camU = g_app->m_camera->GetUp() * size;

        LegacyVector3 statusPos = s_controlPadStatus->GetWorldMatrix( mat ).pos;
        statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
        statusPos.y += 5.0f;

        WorldObjectId occupantId = GetPortOccupant(i);
        if( !occupantId.IsValid() )
        {
            g_imRenderer->Color4ub( 150, 150, 150, 255 );
        }
        else
        {
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            g_imRenderer->Color4ubv( teamColour.GetData() );
        }

        g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
        g_imRenderer->BindTexture(g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
        g_imRenderer->Begin(PRIM_QUADS);
            g_imRenderer->TexCoord2i( 0, 0 );           g_imRenderer->Vertex3fv( (statusPos - camR - camU).GetData() );
            g_imRenderer->TexCoord2i( 1, 0 );           g_imRenderer->Vertex3fv( (statusPos + camR - camU).GetData() );
            g_imRenderer->TexCoord2i( 1, 1 );           g_imRenderer->Vertex3fv( (statusPos + camR + camU).GetData() );
            g_imRenderer->TexCoord2i( 0, 1 );           g_imRenderer->Vertex3fv( (statusPos - camR + camU).GetData() );
        g_imRenderer->End();

        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
        g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
        g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);

    }
}


// ============================================================================


BlueprintRelay::BlueprintRelay()
:   BlueprintBuilding(),
    m_altitude(400.0f)
{
    m_type = Building::TypeBlueprintRelay;

    SetShape( g_app->m_resource->GetShape( "blueprintrelay.shp" ) );
}


void BlueprintRelay::Initialise( Building *_template )
{
    BlueprintBuilding::Initialise( _template );

    BlueprintRelay *blueprintRelay = (BlueprintRelay *) _template;
    m_altitude = blueprintRelay->m_altitude;

    m_pos.y = m_altitude;
    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );
}


void BlueprintRelay::SetDetail( int _detail )
{
    m_pos.y = m_altitude;

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}


bool BlueprintRelay::Advance()
{
    float ourTime = g_gameTime + m_id.GetUniqueId() + m_id.GetIndex();

    LegacyVector3 oldPos = m_pos;

    m_pos.x += sinf( ourTime/1.5f ) * 1.0f;
    m_pos.y += sinf( ourTime/1.3f ) * 1.0f;
    m_pos.z += cosf( ourTime/1.7f ) * 1.0f;

    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;

    m_centrePos = m_pos;

    return BlueprintBuilding::Advance();
}


void BlueprintRelay::Render( float _predictionTime )
{
    BlueprintBuilding::Render( _predictionTime );

    if( g_app->m_editing )
    {
        m_pos.y = m_altitude;
    }
}


void BlueprintRelay::Read( TextReader *_in, bool _dynamic )
{
    BlueprintBuilding::Read( _in, _dynamic );

    m_altitude = atof( _in->GetNextToken() );
}


void BlueprintRelay::Write( FileWriter *_out )
{
    BlueprintBuilding::Write( _out );

    _out->printf( "%-2.2f", m_altitude );
}
