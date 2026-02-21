// Inlines

inline const LegacyVector3 &NetworkUpdate::GetWorldPos() const
{
	return m_teamControls.m_mousePos;
}

inline LegacyVector3 &NetworkUpdate::GetWorldPos() 
{
	return m_teamControls.m_mousePos;
}
