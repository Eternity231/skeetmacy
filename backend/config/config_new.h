#pragma once
#include "../../includes.h"

class CSettings
{
public:
	// returns true/false whether the function succeeds, usually returns false if file doesn't exist
	bool Save( std::string file_name );
	bool Load( std::string file_name );

	void CreateConfig( ); // creates a blank config

	std::vector<std::string> GetConfigs( );

	// ragebot.
	bool	  enable; //
	bool body_aimm;

	bool hitbox_selection;

	bool	  silent;
	int	  selection;
	bool	  fov;
	int		  fov_amount;
	bool hitbox[ 5 ];
	bool hitbox_history[6];
	bool multipoint[4];
	int		  scale;
	int		  body_scale;
	int		  minimal_damage;
	bool	  minimal_damage_hp;
	bool	  penetrate;
	int		  penetrate_minimal_damage;
	bool	  penetrate_minimal_damage_hp;
	bool      knifebot;
	bool	  zeusbot;

	bool kilffed;
	bool rodgol;

	bool hitsotyu_chams;
	int hitsotyu_chams_color[ 4 ] = { 255, 255, 255, 255 };
	int last_tick_key;

	// col2.
	int      zoom;
	bool      nospread;
	bool      norecoil;
	bool      hitchance;
	int	      hitchance_amount;
	bool      lagfix;
	bool	  correct; // 
	//MultiDropdown baim1;
	//MultiDropdown baim2;
	int        baim_hp;
	int       baim_key;

	// antiaim
	bool enable_aa;
	bool edge;
	int mode;

	int pitch_stand;
	int yaw_stand;
	int jitter_range_stand;
	int rot_range_stand;
	int rot_speed_stand;
	int rand_update_stand;
	int dir_stand;
	int dir_time_stand;
	int dir_custom_stand;
	int dir_lock;
	int base_angle_stand;
	int body_fake_stand;

	// col 2.
	int fake_yaw;
	int fake_relative;
	int fake_jitter_range;

	bool      lag_enable;
	//MultiDropdown lag_active;
	int    lag_mode;
	int    lag_limit;
	bool    lag_land;

	// visuals
	//MultiDropdown box;
	bool   box_enemy;
	Color   box_friendly;
	bool      dormant;
	bool      offscreen;
	Color   offscreen_color;
	//MultiDropdown name;
	//MultiDropdown health;
	//MultiDropdown flags_enemy;
	//MultiDropdown flags_friendly;
	//MultiDropdown weapon;
	int      weapon_mode;
	bool      ammo;
	bool      lby_update;
	Color   lby_update_color;

	// col2.
	//MultiDropdown skeleton;
	Color   skeleton_enemy;
	Color   skeleton_friendly;

	//MultiDropdown glow;
	Color   glow_enemy;
	Color   glow_friendly;
	int        glow_blend;
	//MultiDropdown chams_enemy;
	Color   chams_enemy_vis;
	Color   chams_enemy_invis;
	int        chams_enemy_blend;
	bool      chams_enemy_history;
	Color   chams_enemy_history_col;
	int        chams_enemy_history_blend;
	//MultiDropdown chams_friendly;
	Color   chams_friendly_vis;
	Color   chams_friendly_invis;
	int        chams_friendly_blend;
	bool      chams_local;
	Color   chams_local_col;
	int        chams_local_blend;
	bool      chams_local_scope;

	bool      items;
	bool      ammo_dropped;

	bool      proj;
	//Color   proj_color;
	//MultiDropdown proj_range;
	//MultiDropdown planted_c4;
	bool      disableteam;
	int	  world;
	bool      transparent_props;
	bool      enemy_radar;

	// col2.
	bool      novisrecoil;
	bool      nosmoke;
	bool      nofog;
	bool      noflash;
	bool      noscope;
	bool      fov_visuals;
	int        fov_amt;
	bool      fov_scoped;
	bool      viewmodel_fov;
	int        viewmodel_fov_amt;
	bool      spectators;
	bool      force_xhair;
	bool      spread_xhair;
	Color   spread_xhair_col;
	int        spread_xhair_blend;
	bool      pen_crosshair;
	//MultiDropdown indicators;
	bool      tracers;
	bool      impact_beams;
	Color   impact_beams_color;
	Color   impact_beams_hurt_color;
	int        impact_beams_time;
	int       thirdperson;

	int selected_id;
	bool enable_skinchanger;

	int skin_glock;
	int skin_deagle;

	int skin_elite;
	int skin_awp;
	int skin_ssg;
	int skin_auto_ct;
	int skin_auto_t;
	int skin_usp;
	int skin_revolver;
	int skin_tec;
	bool cltang;
	int fakewalkke;

	int weapon_seed;
	int weapon_wear;

	bool custom_health;


	int box_color[ 4 ] = { 255, 255, 255, 255 };
	int name_color[ 4 ] = { 255, 255, 255, 255 };
	int health_color[ 4 ] = { 255, 255, 255, 255 };
	int weapon_color[ 4 ] = { 255, 255, 255, 255 };
	int skeleton_color[ 4 ] = { 255, 255, 255, 255 };
	int ammo_bar_color[ 4 ] = { 255, 255, 255, 255 };

	int chams_color_v[ 4 ] = { 255, 255, 255, 255 };
	int chams_color_iv[ 4 ] = { 255, 255, 255, 255 };

	int local_color_chams[ 4 ] = { 255, 255, 255, 255 };
	int menu_color[ 4 ] = { 255, 255, 255, 255 };
	int proj_color[ 4 ] = { 255, 255, 255, 255 };
	int proj_range_color[ 4 ] = { 255, 255, 255, 255 };
	int item_color[ 4 ] = { 255, 255, 255, 255 };
	bool local_chams;
	bool sliderwalk;
	bool local_blend;

	int autobuyy;

	bool aim_corection;
	bool automatic_hc_increase;

	int knife_changer;


	bool event_item_p;
	bool event_c4;

	bool night_mode;
	bool indicator_enable;
	bool hitmarker;
	bool proj_ranages;
	bool skeleton_esp;

	bool name_esp;
	bool weapon_esp;
	bool esp_icon_weapon;
	bool ammo_esp;
	bool lby_update_esp;
	bool health_esp;
	bool flags_esp;
	bool unlock_inve;
	bool enemy_glow;
	bool bullet_tracers;
	bool removefog;
	bool enemy_chams;
	bool damagelog;

	bool grenade_tracer;
	bool fullbrigan;
	bool force_xharir;
	bool remove_team;
	bool fov_while_scoped;

	bool noflashs;

	int tp_key;
	int key_style;

	int doubletap_key;

	bool dick_hop;
	bool autostopp;
	bool dick_strafe;

	bool vfov;
	int v_fov_am;
	
	bool jump_scouting;
	bool smart_limbs;
	bool wait_for_accurate_shot;
	bool delay_shot_on_unduck;
	bool ignor_limbs_when_moving;
	bool increase_tickcount;
	bool incriminate_stop;
	bool in_airFix;
	bool static_point_scale;

	int overriden_dmg;
	int override_keu;

	int event_mode_type;
	bool debug_reset_logs;

	bool stop_if_failed_hc;
	int override_hc;
	bool hc_debug;
	int override_hc_key;
	int force_baim_key;
	bool reset_if_hurt;
	bool planted_c4;

	int hitbox_prioritize;
	bool sort_target_for_record;
	bool smart_damage_dealing;

	bool money_flag;
	bool armor_flag;
	bool resolver_flag;
	bool hitbox_flag;
	bool scope_flag;
	bool reload_flag;
	bool main_flags;

	bool get_record_speed;
	bool get_2d_speed;

	bool baim_condition_dt;
	bool update_prediction_dt;
	bool interpolate_command;

	bool update_accuray;
	bool lower_tracing;

	int hitchance_mode;

	bool breaker;
	int breaker_key;
	int breaker_side;
	int invert_aa_side;

	int exploit_aa;
	int LBYExploit;

	bool adaptive_fl_onetap;

	// eyepos
	bool fix_eyepos;
	bool rewrite_record_data;

	// immortal
	int imm_body_scale;
	int imm_point_scale;

	// prediction.
	bool violanes_prediction;
	bool pred_logs;

	int pene_type;
	bool penelog;

	bool distortion;
	int distortion_type;
	bool distortion_force_turn;
	int distortion_speed;
	int distortion_angle;

	bool disable_distort_in_air;
	bool disable_distort_when_run;

	int distortion_max_time;
	bool distort_twist;

	bool shift_aa;
	int air_stuck_reverse;

	int aa_right;
	int aa_left;
	int aa_back;

	bool debug_logs;
	bool dont_print;
	bool keep_comunicate;
	bool networking_debug;

	int resolver_type;

	int rage_type;
	int fakeflickanol;

	bool custom_box;
	int box_vertical;
	int box_horizontal;
	
	bool getze_backround_h;
	bool getze_backround_a;

	bool outline_healthbar;
	bool outtline_health_pri;
	bool getze_hp_value;
	bool fullzise_health;
	bool health_animation;
	bool custom_animate;
	int animation_health;

	int lbu_update_color[ 4 ] = { 255, 255, 255, 255 };
	bool outline_healthbar_a;
	bool outtline_health_pri_a;
	int weapon_color_aa[ 4 ] = { 255, 255, 255, 255 };
	bool ammo_show_value_fast;

	bool distance_flag;
	bool location_flag;
	bool shift_flag;

	bool getzeus_scoped;
	bool getzezuz_hitflag;

	int bulettraceroul;
	bool fl_indic;

	int projectile_tiple;
	int bomb_type_ind;

	bool mentain_transition;
	bool fix_setup_bones_timing;
	bool dont_stop_in_air;

	bool lanterna;
	int flashkey;

	int keystyle_left = 1;
	int keystyle_manu_bacl = 1;
	int key_right_style = 1;
	int keystyle_dt;
	int keystyle_ff;
	int keystake_tp;
	int keystake_forcebody;
	int keystanle_damag;

	bool extend_fakelag_packeets;
	int autostop_type;
	bool manualaa;
	bool update_predicto;

	bool precompilseed;
	int accuracy_boost;
	bool delay_ss;
	int delay_shot;

	bool vulerable;

	bool clientt_impact;
	int imps[ 4 ] = { 255, 255, 255, 255 };
	int impc[ 4 ] = { 255, 255, 255, 255 };

	bool dhitmarker;
	int indicator_style;

	bool indicatttors;
	bool speclissst;
	bool keybindddds;
	bool pplllm;

	bool round_summary;
	bool localyra;

	int traiillcolor [4] = { 255, 255, 255, 255 };
	bool manualaaaa;
	bool suneset;
	bool detect_ruberband;
	bool predict_velocity;


	bool enable_stacker;
	bool delay_shortan;
	int dickhead_height;
	bool dickbody_scale;
	bool dick_backtrack;
	int dickdamage;
	bool dickbacktrackpos;
	int dickchance;

	bool ascope;
	int imm_hc;
	bool imm_rage;
	bool imm_static_pointscale;
	int targte_sele;
	bool overideeeunimm;
	int mind_mg_imm;
	int min_keyu;
	bool bodyprefer;
	int mindmgovim;

	bool immfal;
	int immlimit;
	int immmodefl;
	int varianceimm;
	int alternativechok;

	bool jnifebot;
	int modeknife;
	bool izeusan;
	bool izeushc;

private:
}; extern CSettings settings;