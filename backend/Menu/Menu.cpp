ï»¿#include "Menu.h"
/*
#include "../../Backend/Utilities/Utilities.h"
#include "../../Backend/Config/Settings.h"
#include "../../Backend/Config/Config.h"
#include "../../Backend/Features/Visuals/ESP.h"
*/
#include "../../includes.h"
#include <algorithm>



using namespace IdaLovesMe;

/// <summary>
/// Initialize GTX, and preset some stuff.
/// Called only once at the beginning.
/// </summary>
/// 

CMenu::CMenu( ) {
	// nothing there
}

void CMenu::Initialize() 
{
	if (this->m_bInitialized)
		return;

	ui::CreateContext();
	GuiContext* g = Globals::Gui_Ctx;
	g->ItemSpacing = Vec2(0, 6);
	g->MenuAlpha = 1;

	//CConfig::get()->LoadDefaults();

	notify_controller::g_notify.push_event("Cheat Initialized!", "[initialization] " );

	this->m_bIsOpened = true;
	this->m_bInitialized = true;
}

/// <summary>
/// Drawing the user interface.
/// Please keep in mind that this function is called on each frame.
/// </summary>
/// 
/// 
/// 

float GetDeltaTime( ) {
	return g_csgo.m_globals ? g_csgo.m_globals->m_frametime : 0.f;
}

#include "../config/config_new.h"

std::string color_to_string( Color col ) { // test
	return std::to_string( ( col.r() ) ) + "," + std::to_string( ( int )( col.g() ) ) + "," + std::to_string( ( int )( col.b() ) ) + "," + std::to_string( ( int )( col.a() ) );
	/*char hexcol[16];
	snprintf(hexcol, sizeof hexcol, "%02X%02X%02X%02X", col[0], col[1], col[2], col[3]);
	return hexcol;*/
}

void CMenu::Draw()
{
	//CConfig* cfg = CConfig::get();

	static float alpha = 0;
	float fc = GetDeltaTime() * 255 * 10;
	if (!this->m_bIsOpened && alpha > 0.f)
		alpha = std::clamp(alpha - fc / 255.f, 0.f, 1.f);

	if (this->m_bIsOpened && alpha < 1.f)
		alpha = std::clamp(alpha + fc / 255.f, 0.f, 1.f);

	Globals::Gui_Ctx->MenuAlpha = static_cast<int>(floor(alpha * 255));

	if (!this->m_bIsOpened && alpha == 0.f)
		return;

	ui::GetInputFromWindow("Counter-Strike: Global Offensive - Direct3D 9");

	ui::SetNextWindowSize({ 660,560 });
	ui::Begin("Main", GuiFlags_None);

	ui::TabButton("A", &this->m_nCurrentTab, 0, 7);
	ui::TabButton("G", &this->m_nCurrentTab, 1, 7);
	ui::TabButton("B", &this->m_nCurrentTab, 2, 7);
	ui::TabButton("C", &this->m_nCurrentTab, 3, 7);
	ui::TabButton("D", &this->m_nCurrentTab, 4, 7);
	ui::TabButton("E", &this->m_nCurrentTab, 5, 7);
	ui::TabButton("F", &this->m_nCurrentTab, 6, 7);
	ui::TabButton("H", &this->m_nCurrentTab, 7, 7);
	

	switch(this->m_nCurrentTab)
	{
		
		//
		// RAGE
		//
		case 0:
		{
			ui::BeginChild("Other#Rage", { Vec2(6, 0), Vec2(3, 10) });
			{
				ui::Checkbox( "a1", &settings.enable_stacker );



				ui::Checkbox( "Anti-aim correction", &settings.correct );
				ui::SingleSelect( "Resolver type", &settings.resolver_type, { "Default", "Onetap", "Hyperbius" } );
				//ui::Checkbox( "Punch-angle correction", &settings.aim_corection );
				ui::Checkbox( "Lag-compensation fix", &settings.lagfix );
				ui::Checkbox( "Automatic stop", &settings.autostopp );
				ui::SingleSelect( "Automatic stop type", &settings.autostop_type, { "Default", "Getze" } );
				ui::Checkbox( "Dont stop in air", &settings.dont_stop_in_air );
				ui::Checkbox( "Automatic target sorting", &settings.sort_target_for_record, true );
				ui::Checkbox( "Stop if hitchance fail", &settings.stop_if_failed_hc );
				if ( settings.stop_if_failed_hc ) {
					ui::Checkbox( "[Debug] Fail hitchance logs", &settings.hc_debug, true );
				}
				ui::Checkbox( "Smart damage-dealing", &settings.smart_damage_dealing );
				ui::Checkbox( "Dynamic air scale", &settings.in_airFix );
				ui::Checkbox( "Lower hitchance seeds", &settings.lower_tracing );
				ui::Checkbox( "Automatic hitchance increase", &settings.automatic_hc_increase );
				ui::Checkbox( "Detect rubberband", &settings.detect_ruberband, true );
				ui::Checkbox( "Predict velocity", &settings.predict_velocity, true );
				ui::Checkbox( "Jump Scout", &settings.jump_scouting );
				ui::Checkbox( "Delay shot", &settings.delay_ss, true );
				if ( settings.delay_ss ) {
					ui::SliderInt( "Delay ms", &settings.delay_shot, 0, 200, "%d%%" );
				}
				ui::Checkbox( "Delay shot for accuracy", &settings.wait_for_accurate_shot, true );
				ui::Checkbox( "Delay shot on unduck", &settings.delay_shot_on_unduck, true );
				ui::Checkbox( "Smart-limbs", &settings.smart_limbs, true );
				if ( settings.smart_limbs ) {
					ui::Checkbox( "Ignore limbs while moving", &settings.ignor_limbs_when_moving, true );
				}

				ui::Label( "Doubletap key" );
				ui::KeyBind( "Doubletap keyYY", &settings.doubletap_key, &settings.key_style ); 
				ui::Checkbox( "[Condition] Increase tick-count", &settings.increase_tickcount, true );
				ui::Checkbox( "[Condition] Prefer baim", &settings.baim_condition_dt, true );
				ui::Checkbox( "[Condition] Incriminate stop", &settings.incriminate_stop, true );
				ui::Checkbox( "[Condition] Update prediction", &settings.update_prediction_dt, true );
				ui::Checkbox( "[Condition] Interpolate commands", &settings.interpolate_command, true );

				ui::SingleSelect( "Shot informations reset", &settings.event_mode_type, { "Default", "Immortal" } );
				if ( settings.event_mode_type == 1 ) {
					ui::Checkbox( "[Debug] Reset logs", &settings.debug_reset_logs, true );
					ui::Checkbox( "[Condition] Reset if hurt", &settings.reset_if_hurt, true );
				}

				ui::SingleSelect( "Autowall scale-damage system", &settings.pene_type, { "Default", "Fatality" } );
				if ( settings.pene_type == 1 ) {
					ui::Checkbox( "[Debug] Autowall debug", &settings.penelog, true );
				}

				ui::Checkbox( "[Debug] Logs", &settings.debug_logs, true );
				if ( settings.debug_logs )
					ui::Checkbox( "[Debug] Dont render logs", &settings.dont_print );

				ui::Checkbox( "[Networking] Fix reliable transmission", &settings.keep_comunicate );
				if ( settings.keep_comunicate )
					ui::Checkbox( "[Debug] Networking logs", &settings.networking_debug, true );
			}
			ui::EndChild();
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			ui::BeginChild("Aimbot", { Vec2(0, 0), Vec2(3, 10) });
			{
				ui::Checkbox( "Enabled", &settings.enable );
				ui::Checkbox( "Silent", &settings.silent );
				ui::SingleSelect( "Target selection", &settings.selection, { "Distance", "Crosshair", "Damage", "Lowest hp", "Lagcomp", "Height" } );
				ui::Checkbox( "Limit fov", &settings.fov );
				if ( settings.fov ) {
					ui::SliderInt( "Fov amount", &settings.fov_amount, 0, 360, "%d%%" );
				}
				ui::SingleSelect( "Hitbox priority", &settings.hitbox_prioritize, { "Off", "Head", "Baim", "Automatic" } );
				if ( settings.hitbox_prioritize == 3 ) {
					ui::Checkbox( "[Condition] Get record player speed", &settings.get_record_speed, true );
					ui::Checkbox( "[Condition] Get 2D player speed", &settings.get_2d_speed, true );
				}
				ui::Checkbox( "Static point-scale", &settings.static_point_scale );
				if ( settings.static_point_scale ) {
					ui::SliderInt( "Hitbox scale", &settings.scale, 0, 100, "%d%%" );
					ui::SliderInt( "Body scale", &settings.body_scale, 0, 100, "%d%%" );
				}

				ui::Checkbox( "Automatic knife", &settings.knifebot );
				ui::Checkbox( "Automatic zeus", &settings.zeusbot );
				ui::Checkbox( "No recoil", &settings.norecoil );
				ui::Checkbox( "Smart-baim", &settings.body_aimm );
				ui::Checkbox( "Hitchance", &settings.hitchance );
				if ( settings.hitchance ) {
					ui::SingleSelect( "Hitchance type", &settings.hitchance_mode, { "Default", "Immortal", "Getze.us" } );

					ui::Checkbox( "[Condition] Update accuracy penalty", &settings.update_accuray, true );
					ui::Checkbox( "[Condition] Precompile seeds", &settings.precompilseed, true );
					ui::SliderInt( "Minimum hit chance", &settings.hitchance_amount, 0, 100, "%d%%" );

					if ( settings.hitchance_mode == 2 ) {
						ui::SliderInt( "Accuracy boost", &settings.accuracy_boost, 0, 100, "%d%%" );
					}
				}

				ui::SliderInt( "Visible damage", &settings.minimal_damage, 0, 100, "%d%%" );
				ui::SliderInt( "Awall damage", &settings.penetrate_minimal_damage, 0, 100, "%d%%" );

				ui::Label( "Override damage key" );
				ui::KeyBind( "damage keyYY", &settings.override_keu, &settings.key_style );
				ui::SliderInt( "Override damage", &settings.overriden_dmg, 0, 100, "%d%%" );

				ui::Label( "Hitchance key" );
				ui::KeyBind( "Hitchance keyYY", &settings.override_hc_key, &settings.key_style );
				ui::SliderInt( "Override hitchance", &settings.override_hc, 0, 100, "%d%%" );

				ui::Label( "Force baim key" );
				ui::KeyBind( "forcebaimm keyYY", &settings.force_baim_key, &settings.key_style );

			}
			ui::EndChild();

			break;
		}
	
		//
		// AA
		//
		case 1:
		{
			ui::BeginChild("Anti-aimbot angles"); 
			{
				ui::Checkbox( "Enabled", &settings.enable_aa );
				ui::Checkbox( "Edge", &settings.edge );

				ui::SingleSelect( "Pitch", &settings.pitch_stand, { "Off", "Down", "Up", "Random", "Ideal"} );
				ui::SingleSelect( "Yaw base", &settings.yaw_stand, { "Off", "Direction", "Jiier", "Rotate", "Random" } );
				if ( settings.yaw_stand == 1 ) {
					ui::SliderInt( "Jitter angle", &settings.jitter_range_stand, 0, 180, "%d%%" );
				}
				ui::SingleSelect( "Base angle", &settings.base_angle_stand, { "Off", "Static", "Away crosshair", "Away distance" } );
				ui::SingleSelect( "Fake body", &settings.body_fake_stand, { "Off", "Left", "Right", "Opposite","Z" } );

				ui::Checkbox( "Distortion", &settings.distortion );
				ui::SingleSelect( "Distortion type", &settings.distortion_type, { "Dopium", "Onetap", "Immortal" } );
				switch ( settings.distortion_type ) {
					case 0: {
						ui::SliderInt( "Distortion angle", &settings.distortion_angle, 0, 180, "%d%%" );
						ui::SliderInt( "Distortion speed", &settings.distortion_speed, 0, 100, "%d%%" );
					} break;
					case 1: {
						ui::Checkbox( "Force turn", &settings.distortion_force_turn );
						ui::SliderInt( "Distortion angle", &settings.distortion_angle, 0, 180, "%d%%" );
						ui::SliderInt( "Distortion speed", &settings.distortion_speed, 0, 100, "%d%%" );
					} break;
					case 2: {
						ui::Checkbox( "Disable when running", &settings.disable_distort_when_run );
						ui::Checkbox( "Disable when air", &settings.disable_distort_in_air );
						ui::Checkbox( "Twist", &settings.distort_twist );


						ui::SliderInt( "Max time", &settings.distortion_max_time, 0, 20, "%d%%" );
						ui::SliderInt( "Distortion angle", &settings.distortion_angle, 0, 180, "%d%%" );
						ui::SliderInt( "Distortion speed", &settings.distortion_speed, 0, 100, "%d%%" );
					}
				}

				ui::Checkbox( "Manual aa", &settings.manualaa );
				if ( settings.manualaa ) {
					ui::Label( "Manual back key" );
					ui::KeyBind( "fo213rce123124sads6575abaimm keyYY", &settings.aa_back, &settings.keystyle_manu_bacl );

					ui::Label( "Manual right key" );
					ui::KeyBind( "fo213rc1231esadASDsabaimm keyYY", &settings.aa_right, &settings.key_right_style );

					ui::Label( "Manual left key" );
					ui::KeyBind( "fo213rces21adsa1231baimm keyYY", &settings.aa_left, &settings.keystyle_left );
				}
			}
			ui::EndChild();
		
			ui::BeginChild("Fake lag", {Vec2(6,0), Vec2(3, 5)}); {
				ui::Checkbox( "Fake-lag enable", &settings.lag_enable );
				ui::Checkbox( "Fake-lag on land", &settings.lag_land );
				ui::Checkbox( "Onetap adaptive fake-lag", &settings.adaptive_fl_onetap, true );
				ui::Checkbox( "Extend packets", &settings.extend_fakelag_packeets, true );
				ui::SingleSelect( "Lag mode", &settings.lag_mode, { "Max", "Break", "Random", "Break step", "Factor [reversed]", "Switch [reversed]" } );

				ui::SliderInt( "Fake-lag ticks", &settings.lag_limit, 0, settings.extend_fakelag_packeets ? 30 : 15, "%d%%" );
			}
			ui::EndChild();

			ui::BeginChild("Other#AA", { Vec2(6,7), Vec2(3, 3) });
			{
				ui::SingleSelect( "Fake yaw", &settings.fake_yaw, { "Off", "Default", "Relative", "Jitter","Rotate", "Random", "Local view" } );
				if ( settings.fake_yaw == 2 ) {
					ui::SliderInt( "Relative angle", &settings.fake_relative, 0, 180, "%d%%" );
				}

				ui::SingleSelect( "Exploit aa", &settings.exploit_aa, { "Breaker", "Extended", "Shift [!]" } );
				switch ( settings.exploit_aa ) {
					case 0: {
						ui::Checkbox( "Breaker", &settings.breaker );
						if ( settings.breaker ) {
							ui::SliderInt( "Breaker angle", &settings.breaker_side, 0, 180, "%d%%" );

							ui::Label( "Breaker key" );
							ui::KeyBind( "sad123112 keyYY", &settings.breaker_key, &settings.key_style );

							ui::Label( "Breaker invert key" );
							ui::KeyBind( "fo213rcesadsabaimm keyYY", &settings.invert_aa_side, &settings.key_style );

						}
					} break;
					case 1: {
						ui::Label( "Extended key" );
						ui::KeyBind( "s4ad122312112 keyYY", &settings.LBYExploit, &settings.key_style );
					} break;
				}

				ui::Label( "Fake-flick key" );
				ui::KeyBind( "fak1j2y193y1 keyYY", &settings.fakeflickanol, &settings.key_style );

 
			}
			ui::EndChild();

			break;
		}
		
		//
		// LEGIT
		//
		case 2:
		{
			ui::BeginChild("Weapon type#Legit", { Vec2(0,0), Vec2(9, 0) }, GuiFlags_NoMove | GuiFlags_NoResize);
			ui::TabButton("G", &this->m_nCurrentLegitTab, 0, 6, GuiFlags_LegitTab);
			ui::TabButton("P", &this->m_nCurrentLegitTab, 1, 6, GuiFlags_LegitTab);
			ui::TabButton("W", &this->m_nCurrentLegitTab, 2, 6, GuiFlags_LegitTab);
			ui::TabButton("d", &this->m_nCurrentLegitTab, 3, 6, GuiFlags_LegitTab);
			ui::TabButton("f", &this->m_nCurrentLegitTab, 4, 6, GuiFlags_LegitTab);
			ui::TabButton("a", &this->m_nCurrentLegitTab, 5, 6, GuiFlags_LegitTab);
			ui::EndChild();
			ui::BeginChild("Aimbot#Legit", { Vec2(0, 2), Vec2(3, 8) });
			ui::EndChild();
			ui::BeginChild("Triggerbot#Legit", { Vec2(6, 2), Vec2(3, 5) });
			ui::EndChild();
			ui::BeginChild("Other#Legit", { Vec2(6, 9), Vec2(3, 1) });
			ui::EndChild();

			break;
		}
	
		//
		// VFX
		//
		case 3:
		{
			ui::BeginChild("Player ESP", { Vec2(0,0), Vec2(3, 6) }); 
			{
				ui::Checkbox( "Dormancy esp", &settings.dormant );
				ui::Checkbox( "Player offscreen", &settings.offscreen ); // fuck color maybe i will figure out tmr or later
				ui::Checkbox( "Radar", &settings.enemy_radar ); 
				ui::Checkbox( "Bounding box", &settings.box_enemy ); 
				ui::ColorPicker( "box_color", settings.box_color );
				if ( settings.box_enemy ) {
					ui::Checkbox( "Custom box", &settings.custom_box );
					if ( settings.custom_box ) {
						ui::SliderInt( "Box horizontal", &settings.box_horizontal, 0, 100, "%d%%" );
						ui::SliderInt( "Box vertical", &settings.box_vertical, 0, 100, "%d%%" );
					}
				}
				ui::Checkbox( "Name", &settings.name_esp ); 
				ui::ColorPicker( "name_color", settings.name_color );
				ui::Checkbox( "Health", &settings.health_esp ); 
				if ( settings.health_esp ) {
					ui::Checkbox( "Custom health color", &settings.custom_health );
					ui::ColorPicker( "sHEALTH_color", settings.health_color );

					ui::Checkbox( "Fullsize healthbar", &settings.fullzise_health );
					ui::Checkbox( "Healthbar animation", &settings.health_animation );
					if ( settings.health_animation ) {
						ui::Checkbox( "Custom animation", &settings.custom_animate );
						if ( settings.custom_animate ) {
							ui::SliderInt( "Animation value", &settings.animation_health, 0, 200, "%d%%" );
						}
					}
					ui::Checkbox( "Getze backround", &settings.getze_backround_h );
					ui::Checkbox( "Getze hp value", &settings.getze_hp_value );
					ui::Checkbox( "Healthbar outline", &settings.outline_healthbar );
					if ( settings.outline_healthbar ) {
						ui::Checkbox( "Outline priority", &settings.outtline_health_pri );
					}
				}
				ui::Checkbox( "Ammo", &settings.ammo_esp ); 
				ui::ColorPicker( "ammo_color", settings.ammo_bar_color );
				if ( settings.ammo_esp ) {
					ui::Checkbox( "Show value after 1 bullet", &settings.ammo_show_value_fast );
					ui::Checkbox( "Getze backround ammo", &settings.getze_backround_a );
					ui::Checkbox( "Ammo outline", &settings.outline_healthbar_a );
					if ( settings.outline_healthbar ) {
						ui::Checkbox( "Outline priority ammo", &settings.outtline_health_pri_a );
					}
				}

				ui::Checkbox( "Weapon", &settings.weapon_esp ); 
				ui::ColorPicker( "weaponcolor", settings.weapon_color );
				ui::Checkbox( "Weapon icon", &settings.esp_icon_weapon );
				ui::Checkbox( "Skeleton", &settings.skeleton_esp );
				ui::ColorPicker( "skeleton_color", settings.skeleton_color );

				ui::Checkbox( "LBU update bar", &settings.lby_update_esp ); 
				ui::Checkbox( "Flags", &settings.main_flags ); 
				if ( settings.main_flags ) {
					ui::Checkbox( "Armor", &settings.armor_flag );
					ui::Checkbox( "Money", &settings.money_flag );
					ui::Checkbox( "Scoped", &settings.scope_flag );
					if ( settings.scope_flag ) {
						ui::Checkbox( "Turn to getze style", &settings.getzeus_scoped );
					}
					ui::Checkbox( "Reload", &settings.reload_flag );
					ui::Checkbox( "Location", &settings.location_flag );
					ui::Checkbox( "Distance", &settings.distance_flag );
					ui::Checkbox( "Vulnerable", &settings.vulerable );
					ui::Checkbox( "Resolver", &settings.resolver_flag, true );
					ui::Checkbox( "Hitbox prioritized point", &settings.hitbox_flag, true );
					ui::Checkbox( "Shifting", &settings.shift_flag, true );
				}
			}
			ui::EndChild();
			ui::BeginChild("Colored models", { Vec2(0, 8), Vec2(3, 2) });
			{
				ui::Checkbox( "Enemy chams", &settings.enemy_chams );
				if ( settings.enemy_chams ) {
					ui::Label( "Visible chams" );
					ui::ColorPicker( "vchams", settings.chams_color_v );
					ui::Label( "Hidden chams" );
					ui::ColorPicker( "ivcham", settings.chams_color_iv );
				}
				ui::Checkbox( "History chams", &settings.hitsotyu_chams );
				if ( settings.hitsotyu_chams ) {
					ui::ColorPicker( "hhvchams", settings.hitsotyu_chams_color );
				}

				ui::Checkbox( "Local chams", &settings.local_chams );
				if ( settings.local_chams ) {
					ui::Label( "Local color" );
					ui::ColorPicker( "localvcolor", settings.local_color_chams );

					ui::Checkbox( "Blend when scope", &settings.local_blend );
				}
			}
			ui::EndChild();
			ui::BeginChild("Other ESP", { Vec2(6, 0), Vec2(3, 3) });
			{

				ui::Checkbox( "[Getze] Manual arrows", &settings.manualaaaa );
				ui::Checkbox( "[Getze] Windows", &settings.pplllm );
				if ( settings.pplllm ) {
					ui::SingleSelect( "Window style", &settings.indicator_style, { "Simple", "Advanced", "Other" } );
					if ( settings.indicator_style == 0 ) {
						ui::Checkbox( "[Getze] Indicators", &settings.indicatttors );
						ui::Checkbox( "[Getze] Keybinds", &settings.keybindddds );
					}
					else if ( settings.indicator_style == 3 ) {
						ui::Checkbox( "Indicators", &settings.indicator_enable );
						ui::Checkbox( "Spectator list", &settings.spectators );
						ui::Checkbox( "Fakelag indicator", &settings.fl_indic );

					}
					ui::Checkbox( "[Getze] Spectators", &settings.speclissst );
					ui::Checkbox( "[Getze] Round summary", &settings.round_summary );
				}				
				ui::Checkbox( "Local trail", &settings.localyra );
				ui::ColorPicker( "dr123131opp", settings.traiillcolor );
				ui::Checkbox( "Penetration crosshair", &settings.pen_crosshair );
				ui::Checkbox( "Dropped weapons", &settings.items );
				ui::ColorPicker( "dropp", settings.item_color );
				ui::Checkbox( "Dropped ammo", &settings.ammo_dropped );
				ui::Checkbox( "Projectile esp", &settings.proj );
				if ( settings.proj ) {
					ui::SingleSelect( "Projectile type", &settings.projectile_tiple, { "Default", "Getze", "Moneybot", "Pandora" } );
				}
				ui::ColorPicker( "proj_color", settings.proj_color );
				ui::Checkbox( "Projectile ranges", &settings.proj_ranages );
				ui::ColorPicker( "proj_rrcolor", settings.proj_range_color );
				ui::Checkbox( "Planted C4", &settings.planted_c4 );
				ui::Checkbox( "Bullet impacts", &settings.clientt_impact );
				ui::Checkbox( "Bullet tracers", &settings.bullet_tracers );
				if ( settings.bullet_tracers ) {
					ui::SingleSelect( "Tracer material", &settings.autobuyy, { "Default", "Getze" } );
				}
				ui::Checkbox( "Grenade tracer", &settings.grenade_tracer );
				ui::Checkbox( "Force crosshair", &settings.force_xharir );

				
			}
			ui::EndChild();
			ui::BeginChild("Effects", { Vec2(6, 5), Vec2(3, 5) });
			{
				ui::Checkbox( "Remove smoke", &settings.nosmoke );
				ui::Checkbox( "Remove flash", &settings.noflashs );
				ui::Checkbox( "Remove team rendering", &settings.remove_team );
				ui::Checkbox( "Remove scope", &settings.noscope );
				ui::Checkbox( "Sunset", &settings.suneset );
				ui::Checkbox( "Night-mode", &settings.night_mode );
				ui::Checkbox( "Full-bright", &settings.fullbrigan );
				ui::Checkbox( "Hitmarker", &settings.hitmarker );
				ui::Checkbox( "Hitmarker 3D", &settings.dhitmarker );
				ui::Checkbox( "Transparent props", &settings.transparent_props );
				ui::Checkbox( "Field of view", &settings.fov_visuals );
				if ( settings.fov_visuals ) {
					ui::Checkbox( "Use fov while scope", &settings.fov_while_scoped );
					ui::SliderInt( "Field of view amount", &settings.fov_amount, 0, 180, "%d%%" );
				}

				ui::Checkbox( "Viewmodel fov", &settings.vfov );
				if ( settings.vfov ) {
					ui::SliderInt( "Viewmodel fov amount", &settings.v_fov_am, 0, 180, "%d%%" );
				}

				if ( settings.night_mode || settings.transparent_props || settings.fullbrigan ) {
					Visuals::ModulateWorld( );
					// we modulate world if we turn on one of those checkboxes
				}

				ui::Checkbox( "Flash-light", &settings.lanterna );
				if ( settings.lanterna ) {
					ui::Label( "Flash-light key" );
					ui::KeyBind( "lanterna keyYY", &settings.flashkey, &settings.key_style ); // im not sure if that works well
				}

				ui::Label( "Thirdperson key" );
				ui::KeyBind( "Thirdperson keyYY", &settings.tp_key, &settings.key_style ); // im not sure if that works well

				
			}
			ui::EndChild();
			
			break;
		}
	
		//
		// MISCELLANEOUS
		//
		case 4: // the worst part colorpickers
		{
			ui::BeginChild("Miscellaneous", { Vec2(0,0), Vec2(3, 10) });
			{
				ui::SingleSelect( "Automatic buy", &settings.autobuyy, { "Off", "Auto", "Ssg", "Awp" } );
				ui::Checkbox( "Preserve killfeed", &settings.kilffed );
				ui::Checkbox( "Clan-tag", &settings.cltang );
				ui::Checkbox( "Rod-goll force", &settings.rodgol );
				ui::Checkbox( "Unlock inventory in game", &settings.unlock_inve );
				ui::Checkbox( "Damage event", &settings.damagelog );
				if ( settings.damagelog ) {
					ui::Checkbox( "Change style to getze", &settings.getzezuz_hitflag );
				}
				ui::Checkbox( "C4 event", &settings.event_c4 );
				ui::Checkbox( "Purchace event", &settings.event_item_p );

				ui::Checkbox( "Onetap prediction", &settings.violanes_prediction, true );
				if ( settings.violanes_prediction ) {
					ui::Checkbox( "Prediction logs", &settings.pred_logs );
				}

				ui::Checkbox( "[Prediction] Maintain sequence transitions", &settings.mentain_transition, true );
				ui::Checkbox( "[Prediction] Fix setup-bones time", &settings.fix_setup_bones_timing, true );
				//ui::Checkbox( "[Prediction] Fix lag-packets", &settings.update_predicto, true );

				ui::Label( "Airstuck key" );
				ui::KeyBind( "Airstuckxcd keyYY", &settings.air_stuck_reverse, &settings.key_style ); // im not sure if that works well

			}
			ui::EndChild();
			ui::BeginChild("Movement", { Vec2(6, 0), Vec2(3, 4) });
			{
				ui::Checkbox( "Automatic bunnyhop", &settings.dick_hop );
				ui::Checkbox( "Automatic strafe", &settings.dick_strafe );
				ui::Checkbox( "Slide walk", &settings.sliderwalk );
				ui::Label( "Fakewalk key" );
				ui::KeyBind( "Fakewalk keyYY", &settings.fakewalkke, &settings.key_style ); // im not sure if that works well

			}
			ui::EndChild();
			ui::BeginChild("Settings", { Vec2(6, 6), Vec2(3, 4) });
			{
				ui::Label( "Menu color" );
				ui::ColorPicker( "menuvcolor", settings.menu_color );
			}
			ui::EndChild();

			break;
		}
	
		//
		// SKINS & MODELS
		//
		case 5:
		{
			ui::BeginChild("Skinchanger", { Vec2(0,0), Vec2(3, 10) });
			{
				ui::Checkbox( "Enable", &settings.enable_skinchanger );
				
				ui::SliderInt( "Skin quality", &settings.weapon_wear, 0, 100, "%d%%" );
				ui::SliderInt( "Skin seed", &settings.weapon_seed, 0, 255, "%d%%" );

				if ( ui::Button( "Force update" ) ) {
					callbacks::ForceFullUpdate( );
					callbacks::SkinUpdate( );
				}
			}
			ui::EndChild();
			ui::BeginChild("Weapon skin", { Vec2(6, 0), Vec2(3, 10) });
			{
				ui::SingleSelect( "Knife changer", &settings.knife_changer, { XOR( "Off" ), XOR( "Bayonet" ), XOR( "Bowie" ), XOR( "Butterfly" ), XOR( "Falchion" ), XOR( "Flip" ), XOR( "Gut" ), XOR( "Huntsman" ), XOR( "Karambit" ), XOR( "M9 bayonet" ), XOR( "Daggers" ) } );

				if ( g_cl.m_weapon_id == Weapons_t::GLOCK ) {
					if ( ui::BeginListbox( "Glock skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {

						if ( ui::Selectable( "Off", settings.skin_glock == 0 ) ) {
							settings.skin_glock = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_glock == 1 ) ) {
							settings.skin_glock = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_glock == 2 ) ) {
							settings.skin_glock = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_glock == 3 ) ) {
							settings.skin_glock = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_glock == 4 ) ) {
							settings.skin_glock = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_glock == 5 ) ) {
							settings.skin_glock = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_glock == 6 ) ) {
							settings.skin_glock = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_glock == 7 ) ) {
							settings.skin_glock = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_glock == 8 ) ) {
							settings.skin_glock = 8;
						}						
						else if ( ui::Selectable( "Cyrex", settings.skin_glock == 9 ) ) {
							settings.skin_glock = 9;
						}			
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_glock == 10 ) ) {
							settings.skin_glock = 10;
						}						
						else if ( ui::Selectable( "Water Elemental", settings.skin_glock == 11 ) ) {
							settings.skin_glock = 11;
						}						
						else if ( ui::Selectable( "Abyss", settings.skin_glock == 12 ) ) {
							settings.skin_glock = 12;
						}						
						else if ( ui::Selectable( "Dragonfire", settings.skin_glock == 13 ) ) {
							settings.skin_glock = 13;
						}

					}
					ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::DEAGLE ) { 
					if ( ui::BeginListbox( "Deagle skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_deagle == 0 ) ) {
							settings.skin_deagle = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_deagle == 1 ) ) {
							settings.skin_deagle = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_deagle == 2 ) ) {
							settings.skin_deagle = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_deagle == 3 ) ) {
							settings.skin_deagle = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_deagle == 4 ) ) {
							settings.skin_deagle = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_deagle == 5 ) ) {
							settings.skin_deagle = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_deagle == 6 ) ) {
							settings.skin_deagle = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_deagle == 7 ) ) {
							settings.skin_deagle = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_deagle == 8 ) ) {
							settings.skin_deagle = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_deagle == 9 ) ) {
							settings.skin_deagle = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_deagle == 10 ) ) {
							settings.skin_deagle = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_deagle == 11 ) ) {
							settings.skin_deagle = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_deagle == 12 ) ) {
							settings.skin_deagle = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_deagle == 13 ) ) {
							settings.skin_deagle = 13;
						}
					}
					ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::ELITE ) {
					if ( ui::BeginListbox( "Elites skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_elite == 0 ) ) {
							settings.skin_elite = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_elite == 1 ) ) {
							settings.skin_elite = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_elite == 2 ) ) {
							settings.skin_elite = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_elite == 3 ) ) {
							settings.skin_elite = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_elite == 4 ) ) {
							settings.skin_elite = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_elite == 5 ) ) {
							settings.skin_elite = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_elite == 6 ) ) {
							settings.skin_elite = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_elite == 7 ) ) {
							settings.skin_elite = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_elite == 8 ) ) {
							settings.skin_elite = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_elite == 9 ) ) {
							settings.skin_elite = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_elite == 10 ) ) {
							settings.skin_elite = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_elite == 11 ) ) {
							settings.skin_elite = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_elite == 12 ) ) {
							settings.skin_elite = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_elite == 13 ) ) {
							settings.skin_elite = 13;
						}
					}ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::AWP ) {
					if ( ui::BeginListbox( "AWP skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_awp == 0 ) ) {
							settings.skin_awp = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_awp == 1 ) ) {
							settings.skin_awp = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_awp == 2 ) ) {
							settings.skin_awp = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_awp == 3 ) ) {
							settings.skin_awp = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_awp == 4 ) ) {
							settings.skin_awp = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_awp == 5 ) ) {
							settings.skin_awp = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_awp == 6 ) ) {
							settings.skin_awp = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_awp == 7 ) ) {
							settings.skin_awp = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_awp == 8 ) ) {
							settings.skin_awp = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_awp == 9 ) ) {
							settings.skin_awp = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_awp == 10 ) ) {
							settings.skin_awp = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_awp == 11 ) ) {
							settings.skin_awp = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_awp == 12 ) ) {
							settings.skin_awp = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_awp == 13 ) ) {
							settings.skin_awp = 13;
						}
					}ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::SSG08 ) {
					if ( ui::BeginListbox( "Ssg08 skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_ssg == 0 ) ) {
							settings.skin_ssg = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_ssg == 1 ) ) {
							settings.skin_ssg = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_ssg == 2 ) ) {
							settings.skin_ssg = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_ssg == 3 ) ) {
							settings.skin_ssg = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_ssg == 4 ) ) {
							settings.skin_ssg = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_ssg == 5 ) ) {
							settings.skin_ssg = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_ssg == 6 ) ) {
							settings.skin_ssg = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_ssg == 7 ) ) {
							settings.skin_ssg = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_ssg == 8 ) ) {
							settings.skin_ssg = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_ssg == 9 ) ) {
							settings.skin_ssg = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_ssg == 10 ) ) {
							settings.skin_ssg = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_ssg == 11 ) ) {
							settings.skin_ssg = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_ssg == 12 ) ) {
							settings.skin_ssg = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_ssg == 13 ) ) {
							settings.skin_ssg = 13;
						}
					}ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::SCAR20 ) {
					if ( ui::BeginListbox( "Scar20 skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_auto_ct == 0 ) ) {
							settings.skin_auto_ct = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_auto_ct == 1 ) ) {
							settings.skin_auto_ct = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_auto_ct == 2 ) ) {
							settings.skin_auto_ct = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_auto_ct == 3 ) ) {
							settings.skin_auto_ct = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_auto_ct == 4 ) ) {
							settings.skin_auto_ct = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_auto_ct == 5 ) ) {
							settings.skin_auto_ct = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_auto_ct == 6 ) ) {
							settings.skin_auto_ct = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_auto_ct == 7 ) ) {
							settings.skin_auto_ct = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_auto_ct == 8 ) ) {
							settings.skin_auto_ct = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_auto_ct == 9 ) ) {
							settings.skin_auto_ct = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_auto_ct == 10 ) ) {
							settings.skin_auto_ct = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_auto_ct == 11 ) ) {
							settings.skin_auto_ct = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_auto_ct == 12 ) ) {
							settings.skin_auto_ct = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_auto_ct == 13 ) ) {
							settings.skin_auto_ct = 13;
						}
					}ui::EndListbox( );

				}
				else if ( g_cl.m_weapon_id == Weapons_t::G3SG1 ) {
					if ( ui::BeginListbox( "Auto t skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_auto_t == 0 ) ) {
							settings.skin_auto_t = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_auto_t == 1 ) ) {
							settings.skin_auto_t = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_auto_t == 2 ) ) {
							settings.skin_auto_t = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_auto_t == 3 ) ) {
							settings.skin_auto_t = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_auto_t == 4 ) ) {
							settings.skin_auto_t = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_auto_t == 5 ) ) {
							settings.skin_auto_t = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_auto_t == 6 ) ) {
							settings.skin_auto_t = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_auto_t == 7 ) ) {
							settings.skin_auto_t = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_auto_t == 8 ) ) {
							settings.skin_auto_t = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_auto_t == 9 ) ) {
							settings.skin_auto_t = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_auto_t == 10 ) ) {
							settings.skin_auto_t = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_auto_t == 11 ) ) {
							settings.skin_auto_t = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_auto_t == 12 ) ) {
							settings.skin_auto_t = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_auto_t == 13 ) ) {
							settings.skin_auto_t = 13;
						}
					}
					ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::USPS ) {
					if ( ui::BeginListbox( "Usp skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_usp == 0 ) ) {
							settings.skin_usp = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_usp == 1 ) ) {
							settings.skin_usp = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_usp == 2 ) ) {
							settings.skin_usp = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_usp == 3 ) ) {
							settings.skin_usp = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_usp == 4 ) ) {
							settings.skin_usp = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_usp == 5 ) ) {
							settings.skin_usp = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_usp == 6 ) ) {
							settings.skin_usp = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_usp == 7 ) ) {
							settings.skin_usp = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_usp == 8 ) ) {
							settings.skin_usp = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_usp == 9 ) ) {
							settings.skin_usp = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_usp == 10 ) ) {
							settings.skin_usp = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_usp == 11 ) ) {
							settings.skin_usp = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_usp == 12 ) ) {
							settings.skin_usp = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_usp == 13 ) ) {
							settings.skin_usp = 13;
						}
					}
					ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::REVOLVER ) {
					if ( ui::BeginListbox( "Revolver skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_revolver == 0 ) ) {
							settings.skin_revolver = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_revolver == 1 ) ) {
							settings.skin_revolver = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_revolver == 2 ) ) {
							settings.skin_revolver = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_revolver == 3 ) ) {
							settings.skin_revolver = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_revolver == 4 ) ) {
							settings.skin_revolver = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_revolver == 5 ) ) {
							settings.skin_revolver = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_revolver == 6 ) ) {
							settings.skin_revolver = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_revolver == 7 ) ) {
							settings.skin_revolver = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_revolver == 8 ) ) {
							settings.skin_revolver = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_revolver == 9 ) ) {
							settings.skin_revolver = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_revolver == 10 ) ) {
							settings.skin_revolver = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_revolver == 11 ) ) {
							settings.skin_revolver = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_revolver == 12 ) ) {
							settings.skin_revolver = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_revolver == 13 ) ) {
							settings.skin_revolver = 13;
						}
					}ui::EndListbox( );
				}
				else if ( g_cl.m_weapon_id == Weapons_t::TEC9 ) {
					if ( ui::BeginListbox( "Tec skin", IdaLovesMe::Vec2( 70, 140 ) ) ) {
						if ( ui::Selectable( "Off", settings.skin_tec == 0 ) ) {
							settings.skin_tec = 0;
						}
						else if ( ui::Selectable( "Candy Apple", settings.skin_tec == 1 ) ) {
							settings.skin_tec = 1;
						}
						else if ( ui::Selectable( "Westland", settings.skin_tec == 2 ) ) {
							settings.skin_tec = 2;
						}
						else if ( ui::Selectable( "Gamma Doppler", settings.skin_tec == 3 ) ) {
							settings.skin_tec = 3;
						}
						else if ( ui::Selectable( "Doppler 1", settings.skin_tec == 4 ) ) {
							settings.skin_tec = 4;
						}
						else if ( ui::Selectable( "Doppler 2", settings.skin_tec == 5 ) ) {
							settings.skin_tec = 5;
						}
						else if ( ui::Selectable( "Doppler 3", settings.skin_tec == 6 ) ) {
							settings.skin_tec = 6;
						}
						else if ( ui::Selectable( "Doppler 4", settings.skin_tec == 7 ) ) {
							settings.skin_tec = 7;
						}
						else if ( ui::Selectable( "Whiteout", settings.skin_tec == 8 ) ) {
							settings.skin_tec = 8;
						}
						else if ( ui::Selectable( "Cyrex", settings.skin_tec == 9 ) ) {
							settings.skin_tec = 9;
						}
						else if ( ui::Selectable( "Twilight Galaxy", settings.skin_tec == 10 ) ) {
							settings.skin_tec = 10;
						}
						else if ( ui::Selectable( "Water Elemental", settings.skin_tec == 11 ) ) {
							settings.skin_tec = 11;
						}
						else if ( ui::Selectable( "Abyss", settings.skin_tec == 12 ) ) {
							settings.skin_tec = 12;
						}
						else if ( ui::Selectable( "Dragonfire", settings.skin_tec == 13 ) ) {
							settings.skin_tec = 13;
						}
					}
					ui::EndListbox( );
				}

				// add arctic camo
			}
			ui::EndChild();

			break;
		}
	
		//
		// PLAYERS
		//	btw, someone fix tab icon thx (if not fixed yet) -mtfy
		//
		case 6:
		{
			ui::BeginChild("Players", { Vec2(0,0), Vec2(3, 10) });
			ui::EndChild();
			ui::BeginChild("Adjustments", { Vec2(6, 0), Vec2(3, 10) });
			ui::EndChild();

			break;
		}
		
		//
		// CFG & LUA
		//
		case 7:
		{
			ui::BeginChild("Presets", { Vec2(0,0), Vec2(3, 10) });

			// std::vector<std::string> configs = { "default", "legit", "autos", "scouts", "pistols" };

			/* thats just an example on how do to config slots
				if u want more slots do it yourself
			*/

			static int config_slot;
			std::string config_name;

			static bool active1 = false;
			static bool active2 = false;
			static bool active3 = false;

			if (ui::BeginListbox("ConfigsList")) 
			{
				if ( ui::Selectable( "Default", active1 ) ) {
					config_slot = 0; // since that 0/ first
					active1 = true;
					active2 = false;
					active3 = false;
				}
				else if ( ui::Selectable( "Auto", active2 ) ) {
					config_slot = 1;
					active1 = false;
					active2 = true;
					active3 = false;	
				}
			}
			ui::EndListbox();

			switch ( config_slot ) {
				case 0: {
					config_name = "default";
				} break;
				case 1: {
					config_name = "auto_cfg";
				} break;
			}

			// thats the real truth
			if ( ui::Button( "Save" ) ) {
				settings.Save( config_name );
			}

			if ( ui::Button( "Load" ) ) {
				settings.Load( config_name );
			}

			ui::EndChild();
			ui::BeginChild("Lua", { Vec2(6, 0), Vec2(3, 10) });
			ui::EndChild();

			break;
		}
	
		case 8:
		{
			ui::BeginChild("A", { Vec2(0,0), Vec2(3, 10) });
			ui::EndChild();
			ui::BeginChild("B", { Vec2(6, 0), Vec2(3, 10) });
			ui::EndChild();
		}
	}

	ui::End();
}

bool CMenu::IsMenuOpened() 
{
	return this->m_bIsOpened;
}

void CMenu::SetMenuOpened(bool v) 
{
	this->m_bIsOpened = v;
}

D3DCOLOR to_color_d3d( int col[ 4 ] ) {
	return D3DCOLOR_RGBA( col[ 0 ], col[ 1 ], col[ 2 ], 255 );
}

D3DCOLOR CMenu::GetMenuColor() {
	GuiContext* g = Globals::Gui_Ctx;
	//CConfig* cfg = CConfig::get();
	return to_color_d3d(settings.menu_color);//D3DCOLOR_RGBA(cfg->c["MenuColor"][0], cfg->c["MenuColor"][1], cfg->c["MenuColor"][2], min(cfg->c["MenuColor"][3], g->MenuAlpha));
}