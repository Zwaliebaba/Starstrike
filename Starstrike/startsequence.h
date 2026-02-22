
#ifndef _included_startsequence_h
#define _included_startsequence_h


struct ID3D11Buffer;
class StartSequenceCaption;


class StartSequence
{
protected:
    float m_startTime;
    LList<StartSequenceCaption *> m_captions;

    // D3D11 resources
    ID3D11Buffer* m_overlayVB;
    ID3D11Buffer* m_cursorVB;
    ID3D11Buffer* m_gridVB;
    ID3D11Buffer* m_cbPerDraw;

    void RegisterCaption( char *_caption, float _x, float _y, float _size,
                          float _startTime, float _endTime );
    void InitD3DResources();
    void ShutdownD3DResources();

public:
    StartSequence();
    ~StartSequence();

    bool Advance();
    void Render();
};



class StartSequenceCaption
{
public:
    char *m_caption;
    float m_x;
    float m_y;
    float m_size;
    float m_startTime;
    float m_endTime;

};


#endif