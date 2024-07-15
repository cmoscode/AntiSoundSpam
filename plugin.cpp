#include <ISvenModAPI.h>
#include <interface.h>
#include <IClientPlugin.h>
#include <IClientHooks.h>

#include <convar.h>
#include <dbg.h>

#include <data_struct/hash.h>

//-----------------------------------------------------------------------------
// AntiSpam structure
//-----------------------------------------------------------------------------

struct anti_sound_spam_t
{
	anti_sound_spam_t() : entindex(0), blocktime(0.f), lastplayed(0.f)
	{
		sound[ 0 ] = '\0';
	}

	int entindex;
	char sound[ 64 ];

	float blocktime;
	float lastplayed;
};

//-----------------------------------------------------------------------------
// ConVars
//-----------------------------------------------------------------------------

ConVar snd_antispam_enable( "snd_antispam_enable", "1", FCVAR_CLIENTDLL, "Enable anti sound spam from studio events" );
ConVar snd_antispam_min_diff_time( "snd_antispam_min_diff_time", "0.01", FCVAR_CLIENTDLL, "Min. time difference between same sounds to be blocked" );
ConVar snd_antispam_max_diff_time( "snd_antispam_max_diff_time", "0.1", FCVAR_CLIENTDLL, "Max. time difference between same sounds to be blocked" );
ConVar snd_antispam_block_time( "snd_antispam_block_time", "5", FCVAR_CLIENTDLL, "Block time" );
ConVar snd_antispam_remove_delay( "snd_antispam_remove_delay", "60", FCVAR_CLIENTDLL, "Remove a sound when it has been played long time ago" );

//-----------------------------------------------------------------------------
// Plugin interface
//-----------------------------------------------------------------------------
class CAntiSoundSpam : public IClientPlugin, IClientHooks
{
public:
	CAntiSoundSpam();

	// IClientHooks interface
	virtual HOOK_RESULT HUD_VidInit(void);
	virtual HOOK_RESULT HUD_Redraw(float time, int intermission);
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_UpdateClientData(int *changed, client_data_t *pcldata, float flTime);
	virtual HOOK_RESULT HUD_PlayerMove(playermove_t *ppmove, int server);
	virtual HOOK_RESULT IN_ActivateMouse(void);
	virtual HOOK_RESULT IN_DeactivateMouse(void);
	virtual HOOK_RESULT IN_MouseEvent(int mstate);
	virtual HOOK_RESULT IN_ClearStates(void);
	virtual HOOK_RESULT IN_Accumulate(void);
	virtual HOOK_RESULT CL_CreateMove(float frametime, usercmd_t *cmd, int active);
	virtual HOOK_RESULT HOOK_RETURN_VALUE CL_IsThirdPerson(int *thirdperson);
	virtual HOOK_RESULT HOOK_RETURN_VALUE KB_Find(kbutton_t **button, const char *name);
	virtual HOOK_RESULT CAM_Think(void);
	virtual HOOK_RESULT V_CalcRefdef(ref_params_t *pparams);
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_AddEntity(int *visible, int type, cl_entity_t *ent, const char *modelname);
	virtual HOOK_RESULT HUD_CreateEntities(void);
	virtual HOOK_RESULT HUD_DrawNormalTriangles(void);
	virtual HOOK_RESULT HUD_DrawTransparentTriangles(void);
	virtual HOOK_RESULT HUD_StudioEvent(const mstudioevent_t *studio_event, const cl_entity_t *entity);
	virtual HOOK_RESULT HUD_PostRunCmd(local_state_t *from, local_state_t *to, usercmd_t *cmd, int runfuncs, double time, unsigned int random_seed);
	virtual HOOK_RESULT HUD_TxferLocalOverrides(entity_state_t *state, const clientdata_t *client);
	virtual HOOK_RESULT HUD_ProcessPlayerState(entity_state_t *dst, const entity_state_t *src);
	virtual HOOK_RESULT HUD_TxferPredictionData(entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd);
	virtual HOOK_RESULT Demo_ReadBuffer(int size, unsigned const char *buffer);
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_ConnectionlessPacket(int *valid_packet, netadr_t *net_from, const char *args, const char *response_buffer, int *response_buffer_size);
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_GetHullBounds(int *hullnumber_exist, int hullnumber, float *mins, float *maxs);
	virtual HOOK_RESULT HUD_Frame(double time);
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_Key_Event(int *process_key, int down, int keynum, const char *pszCurrentBinding);
	virtual HOOK_RESULT HUD_TempEntUpdate(double frametime, double client_time, double cl_gravity, TEMPENTITY **ppTempEntFree, TEMPENTITY **ppTempEntActive, int (*Callback_AddVisibleEntity)(cl_entity_t *pEntity), void (*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp));
	virtual HOOK_RESULT HOOK_RETURN_VALUE HUD_GetUserEntity(cl_entity_t **ent, int index);
	virtual HOOK_RESULT HUD_VoiceStatus(int entindex, qboolean bTalking);
	virtual HOOK_RESULT HUD_DirectorMessage(unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags);
	virtual HOOK_RESULT HUD_ChatInputPosition(int *x, int *y);

	// IClientPlugin interface
	virtual api_version_t GetAPIVersion();

	virtual bool Load(CreateInterfaceFn pfnSvenModFactory, ISvenModAPI *pSvenModAPI, IPluginHelpers *pPluginHelpers);
	virtual void PostLoad(bool bGlobalLoad);
	virtual void Unload(void);
	virtual bool Pause(void);
	virtual void Unpause(void);

	virtual void OnFirstClientdataReceived(client_data_t *pcldata, float flTime);
	virtual void OnBeginLoading(void);
	virtual void OnEndLoading(void);
	virtual void OnDisconnect(void);

	virtual void GameFrame(client_state_t state, double frametime, bool bPostRunCmd);

	virtual void Draw(void);
	virtual void DrawHUD(float time, int intermission);

	virtual const char *GetName(void);
	virtual const char *GetAuthor(void);
	virtual const char *GetVersion(void);
	virtual const char *GetDescription(void);
	virtual const char *GetURL(void);
	virtual const char *GetDate(void);
	virtual const char *GetLogTag(void);

private:
	static bool HashMap_Compare(const anti_sound_spam_t &a, const anti_sound_spam_t &b) { return (a.entindex == b.entindex && strcmp(a.sound, b.sound) == 0); }
	static unsigned int HashMap_Hash(const anti_sound_spam_t &a) { return HashStringCaseless(a.sound); }

private:
	CHash<anti_sound_spam_t> m_SoundsList;
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CAntiSoundSpam::CAntiSoundSpam() : m_SoundsList(511, HashMap_Compare, HashMap_Hash)
{
}

//-----------------------------------------------------------------------------
// Client DLL dummy hooks
//-----------------------------------------------------------------------------

HOOK_RESULT CAntiSoundSpam::HUD_Redraw( float time, int intermission ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_UpdateClientData( int *changed, client_data_t *pcldata, float flTime ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_PlayerMove( playermove_t *ppmove, int server ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::IN_ActivateMouse( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::IN_DeactivateMouse( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::IN_MouseEvent( int mstate ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::IN_ClearStates( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::IN_Accumulate( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::CL_CreateMove( float frametime, usercmd_t *cmd, int active ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::CL_IsThirdPerson( int *thirdperson ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::KB_Find( kbutton_t **button, const char *name ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::CAM_Think( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::V_CalcRefdef( ref_params_t *pparams ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_AddEntity( int *visible, int type, cl_entity_t *ent, const char *modelname ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_CreateEntities( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_DrawNormalTriangles( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_DrawTransparentTriangles( void ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_PostRunCmd( local_state_t *from, local_state_t *to, usercmd_t *cmd, int runfuncs, double time, unsigned int random_seed ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_TxferLocalOverrides( entity_state_t *state, const clientdata_t *client ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_ProcessPlayerState( entity_state_t *dst, const entity_state_t *src ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_TxferPredictionData( entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::Demo_ReadBuffer( int size, unsigned const char *buffer ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_ConnectionlessPacket( int *valid_packet, netadr_t *net_from, const char *args, const char *response_buffer, int *response_buffer_size ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_GetHullBounds( int *hullnumber_exist, int hullnumber, float *mins, float *maxs ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_Frame( double time ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_Key_Event( int *process_key, int down, int keynum, const char *pszCurrentBinding ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, TEMPENTITY **ppTempEntFree, TEMPENTITY **ppTempEntActive, int ( *Callback_AddVisibleEntity )( cl_entity_t *pEntity ), void ( *Callback_TempEntPlaySound )( TEMPENTITY *pTemp, float damp ) ) { return HOOK_CONTINUE; }
HOOK_RESULT HOOK_RETURN_VALUE CAntiSoundSpam::HUD_GetUserEntity( cl_entity_t **ent, int index ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_VoiceStatus( int entindex, qboolean bTalking ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_DirectorMessage( unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags ) { return HOOK_CONTINUE; }
HOOK_RESULT CAntiSoundSpam::HUD_ChatInputPosition( int *x, int *y ) { return HOOK_CONTINUE; }

//-----------------------------------------------------------------------------
// Client DLL hooks
//-----------------------------------------------------------------------------

HOOK_RESULT CAntiSoundSpam::HUD_VidInit( void )
{
	m_SoundsList.Clear();

	return HOOK_CONTINUE;
}

HOOK_RESULT CAntiSoundSpam::HUD_StudioEvent( const mstudioevent_t *studio_event, const cl_entity_t *entity )
{
	if ( snd_antispam_enable.GetBool() && studio_event->event == CL_EVENT_SOUND && studio_event->options[ 0 ] != '\0' )
	{
		anti_sound_spam_t dummy;
		dummy.entindex = entity->index;
		strncpy_s( dummy.sound, M_ARRAYSIZE( anti_sound_spam_t::sound ), studio_event->options, M_ARRAYSIZE( mstudioevent_t::options ) );

		float flTime = g_pEngineFuncs->GetClientTime();
		anti_sound_spam_t *sound = m_SoundsList.Find( dummy );

		if ( sound != NULL )
		{
			float flTimeDifference = flTime - sound->lastplayed;

			if ( sound->blocktime >= flTime )
			{
				return HOOK_STOP;
			}
			else if ( flTimeDifference >= snd_antispam_min_diff_time.GetFloat() && flTimeDifference <= snd_antispam_max_diff_time.GetFloat() )
			{
				sound->blocktime = flTime + snd_antispam_block_time.GetFloat();
				Msg( "[AntiSoundSpam] Blocked sound: %s (ent: %d)\n", studio_event->options, entity->index );

				return HOOK_STOP;
			}
			else
			{
				sound->lastplayed = flTime;
			}
		}
		else
		{
			// Not dummy now!
			dummy.lastplayed = flTime;
			dummy.blocktime = 0.f;

			m_SoundsList.Insert( dummy );
		}

		//Msg("[studio_event] ent: %d | frame: %d | type: %d | options: %s\n", entity->index, studio_event->frame, studio_event->type, studio_event->options);
	}

	return HOOK_CONTINUE;
}

//-----------------------------------------------------------------------------
// Plugin's implementation
//-----------------------------------------------------------------------------

api_version_t CAntiSoundSpam::GetAPIVersion()
{
	return SVENMOD_API_VER;
}

bool CAntiSoundSpam::Load( CreateInterfaceFn pfnSvenModFactory, ISvenModAPI *pSvenModAPI, IPluginHelpers *pPluginHelpers )
{
	BindApiToGlobals( pSvenModAPI );

	Hooks()->RegisterClientHooks( this );

	g_pEngineFuncs->ClientCmd( "exec antisoundspam.cfg" );

	ConVar_Register();

	ConColorMsg( { 40, 255, 40, 255 }, "[AntiSoundSpam] Successfully loaded\n" );
	return true;
}

void CAntiSoundSpam::PostLoad( bool bGlobalLoad )
{
}

void CAntiSoundSpam::Unload( void )
{
	ConVar_Unregister();

	Hooks()->UnregisterClientHooks( this );
}

bool CAntiSoundSpam::Pause( void )
{
	return true;
}

void CAntiSoundSpam::Unpause( void )
{
}

void CAntiSoundSpam::OnFirstClientdataReceived( client_data_t *pcldata, float flTime )
{
}

void CAntiSoundSpam::OnBeginLoading( void )
{
}

void CAntiSoundSpam::OnEndLoading( void )
{
}

void CAntiSoundSpam::OnDisconnect( void )
{
}

void CAntiSoundSpam::GameFrame( client_state_t state, double frametime, bool bPostRunCmd )
{
	if ( snd_antispam_enable.GetBool() && !bPostRunCmd && state == CLS_ACTIVE )
	{
		float flTime = g_pEngineFuncs->GetClientTime();

		for ( int i = 0; i < m_SoundsList.Count(); i++ )
		{
			HashIterator_t it = m_SoundsList.First( i );

			while ( m_SoundsList.IsValidIterator( it ) )
			{
				anti_sound_spam_t &sound = m_SoundsList.At( i, it );

				if ( flTime - sound.lastplayed >= snd_antispam_remove_delay.GetFloat() && flTime < sound.blocktime )
				{
					m_SoundsList.Remove( sound );
					continue;
				}

				it = m_SoundsList.Next( i, it );
			}
		}
	}
}

void CAntiSoundSpam::Draw( void )
{
}

void CAntiSoundSpam::DrawHUD( float time, int intermission )
{
}

const char *CAntiSoundSpam::GetName( void )
{
	return "AntiSoundSpam";
}

const char *CAntiSoundSpam::GetAuthor( void )
{
	return "Sw1ft";
}

const char *CAntiSoundSpam::GetVersion( void )
{
	return "1.0.0";
}

const char *CAntiSoundSpam::GetDescription( void )
{
	return "Prevents sound spam from studio event animations";
}

const char *CAntiSoundSpam::GetURL( void )
{
	return "https://github.com/sw1ft747/AntiSoundSpam";
}

const char *CAntiSoundSpam::GetDate( void )
{
	return SVENMOD_BUILD_TIMESTAMP;
}

const char *CAntiSoundSpam::GetLogTag( void )
{
	return "AntiSoundSpam";
}

//-----------------------------------------------------------------------------
// Export plugin
//-----------------------------------------------------------------------------

EXPOSE_SINGLE_INTERFACE( CAntiSoundSpam, IClientPlugin, CLIENT_PLUGIN_INTERFACE_VERSION );