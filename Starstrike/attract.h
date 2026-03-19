#pragma once

class AttractMode
{
public:
	char	m_userProfile[256];
	bool	m_running;

public:
	AttractMode();

	void Advance();
	void StartAttractMode();
	void EndAttractMode();

};

