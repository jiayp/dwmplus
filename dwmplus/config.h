/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[] = "WenQuanYi Micro Hei:size=11"; //定义字体

#define NUMCOLORS	8	//定义程序中使用几种颜色
static const char colors[NUMCOLORS][ColLast][8]	= {
    // border   foreground  background
    //配色方案一(边、字色、底色)
    /*{ "#cccccc", "#ffffff", "#000000" },    // 0         = normal 白字黑底,正常使用的颜色，系统托盘也是使用这种颜色
    { "#0000ff", "#000000", "#00ff00" },	// 1	     = selected
    { "#ff6600", "#ffffff", "#ff6600" },	// 2	     = 红底白字，修饰状态栏信息使用
    { "#ffffff", "#00BFFF", "#000000" },	// 3	     = 淡蓝色字黑底,目前用来修饰状态栏文本颜色
    { "#ffffff", "#00ff00", "#000000" },	// 4	     = 黑底绿字，修饰状态栏信息使用
    { "#ffffff", "#000000", "#FFFF00" },	// 5	     = 黄底黑字，修饰状态栏信息使用
    { "#ffffff", "#33ffff", "#000000" },	// 6	     = 网络上传
    { "#ffffff", "#00ff00", "#000000" },	// 7	     = window title text 窗口标题颜色#cae682*/
  
  //配色方案二(边、字色、底色)
   { "#cccccc", "#000000", "#dddddd" }, // 0 = normal 正常使用的颜色，系统托盘也是使用这种颜色
   { "#0000ff", "#ffffff", "#cc6600" }, // 1 = selected被选择的标签页，其中边的颜色指获得焦点的窗口的包围颜色
   { "#ff6600", "#ffffff", "#ff6600" }, // 2 = 红底白字，CPU利用率大于80％
   { "#ffffff", "#00BFFF", "#dddddd" }, // 3 = 信息之间的"|"
   { "#ffffff", "#0000ff", "#dddddd" }, // 4 = 时间、电源、网络下载
   { "#ffffff", "#000000", "#ffff00" }, // 5 = CPU利用率大于60％
   { "#ffffff", "#cc3300", "#dddddd" }, // 6 = 网络上传
   { "#ffffff", "#000000", "#cae682" }, // 7 = window title text 窗口标题颜色(#cae682)*/
};

static const unsigned int borderpx           = 1; /* border pixel of windows 边的宽度*/
static const unsigned int snap               = 32; /* snap pixel */
static const Bool         showbar            = True; /* False means no bar 是否显示面板*/
static const Bool         topbar             = True; /* False means bottom bar 面板的显示位置：上或下*/

/* 系统托盘相关 */
// 是否允许系统托盘
static const Bool systray_enable  = True;
// 托盘图标之间的间隔
static const int  systray_spacing = 2;

/* 状态栏的高度 注意：当状态栏的高度值小于字体的高度时，以字体的高度为准，防止文字显示被遮盖*/
static const int status_height = 0;

/* 标签页tagging */
static const char *tags[]	= { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

//自动隐藏没有内容的tag（Hide empty tags in tag list）
Bool autohide    = True;

//自定义一些程序的默认展现方式，注意程序的第一个字母必须为大写，否则无效！
static const Rule rules[] = {
  /* class       instance    title       tags mask     isfloating   monitor */
    
  // 始终在标签3上打开firefox
   //{ "Firefox",   NULL,        NULL,       1 << 2,    False,      -1 },

  // 始终在标签2上打开emacs；窗口浮动，否则emacs记忆光标的位置会出问题
   //{ "Emacs",   NULL,        NULL,       1 << 1,    True ,      -1 },
  
  // 下列程序打开时处于浮动状态
  { "DoubleBall_QT", NULL,    NULL,       0,       	   True,        -1 }, 
  { "Stardict",      NULL,    NULL,       0,           True,        -1 },
  { "Rhythmbox",      NULL,   NULL,       0,           True,        -1 },
  { "Smplayer",      NULL,   NULL,        0,           True,        -1 },
  { "Vlc",           NULL,    NULL,       0,           True,        -1 },
  { "Volumeicon",    NULL,    NULL,       0,           True,        -1 },
  { "VirtualBox",    NULL,	  NULL,		  1<<3,		   True,		-1 },
  { "Pidgin",        NULL,    NULL,       0,           True,        -1 },
  { "Thunderbird",   NULL,    NULL,       1<<4,        True,        -1 },
  { "Gimp",          NULL,    NULL,       0,           True,        -1 },
  { "Emacs",          NULL,    NULL,       0,           True,        -1 },
  { "Iptux",         NULL,    NULL,       0,           True,        -1 },
  { "Amsn",         NULL,    NULL,       0,            True,        -1 },
  { "Wine",         NULL,    NULL,       0,            True,        -1 },

};

/* layout(s) */
static const float mfact       = 0.55; /* 窗口开启时所占的屏幕比例，默认为0.55 factor of master area size [0.05..0.95] */
static const Bool  resizehints = False; /*如果为True则某些应用程序不能最大化地占据屏幕，比如：xterm等。(True means respect size hints in tiled resizals )*/
// 定义窗口的展现（平铺）方式，具体的实现在诸如tile等函数里面
static const Layout layouts[]  = {
    /* symbol     arrange function */
    { "[M]",       monocle },    //缺省：单个窗口最大化显示
    { "><>",      NULL },        //保持窗口的原始风貌（no layout function means floating behavior）
    { "[]=",        tile },      //左右平铺 
    { "[=]",        bstackhoriz }, //水平上下平铺
    { "[#]" ,       grid },
    { "[||]" ,       bstack },
};

/* key definitions */
#define MODKEY Mod4Mask // use windows key
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view_layout,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands 定义各种命令，以便用快捷键调用*/
// -b 表示dmenu在屏幕下方显示
static const char *dmenucmd[] = { "dmenu_run" , "-p" , "Run:" , "-fn", font, "-nb", colors[0][ColBG], "-nf", colors[0][ColFG], "-sb", colors[1][ColBG], "-sf", colors[1][ColFG], NULL };
static const char *xkill[]    = { "xkill", NULL };
static const char *termcmd[]  = { "terminal", NULL };
static const char *stardict[]  = { "stardict", NULL };
static const char *lockscreencmd[]  = { "slock", NULL };
static const char *browsercmd[]     = { "google-chrome", NULL };
static const char *browseraltcmd[]  = { "firefox", NULL };
static const char *mailcmd[]        = { "thunderbird", NULL };
static const char *filemgrcmd[]     = { "xfe", NULL };
static const char *netmgrcmd[]      = { "xterm", "-e", "wicd-curses", NULL };
static const char *screenshotcmd[]  = { "scrot", NULL };
static const char *musicplay[]      = { "muss", "-p", NULL };
static const char *musicstop[]      = { "muss", "-s", NULL };
static const char *musicnext[]      = { "muss", "-n", NULL };
static const char *musicback[]      = { "muss", "-b", NULL };

static const char *musickcrwalt[]   = { "muss", "kcrw-music", NULL };
static const char *volumeup[]       = { "amixer", "-q", "sset", "Master", "1+", NULL };
static const char *volumedown[]     = { "amixer", "-q", "sset", "Master", "1-", NULL };
static const char *volumemute[]     = { "amixer", "-q", "sset", "Master", "toggle", NULL };

//快捷键定义
static Key keys[] = {
	/* modifier                     key        function        argument */
	{ Mod1Mask|ControlMask,         XK_Delete, spawn,          SHCMD("sudo reboot") },	//重启
	{ Mod1Mask|ControlMask,         XK_End,    spawn,          SHCMD("sudo shutdown -h now") },	//关机
	{ MODKEY,                       XK_Print,  spawn,          SHCMD("scrot %Y%m%d-%H.%M.%S.png -t 280x175 -e 'mv $f $m ~/tmp/'") },
	{ MODKEY|ShiftMask,             XK_Print,  spawn,          SHCMD("scrot %Y%m%d-%H.%M.%S.png -t 280x175 -d 5 -e 'mv $f $m ~/tmp/'") },
	//调用dmenu
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },	
	//执行xkill
	{ MODKEY|ShiftMask,             XK_Escape, spawn,          {.v = xkill } },
	{ MODKEY,                       XK_o,      spawn,          SHCMD("geany")  },	
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },	
	{ MODKEY,                       XK_q,      spawn,          {.v = lockscreencmd } },
	{ MODKEY,                       XK_w,      spawn,          {.v = browsercmd } },	
	{ MODKEY|ShiftMask,             XK_w,      spawn,          {.v = browseraltcmd } },		
	{ MODKEY|ShiftMask,             XK_m,      spawn,          {.v = mailcmd} },		
	//执行文件管理器thunar
	{ MODKEY,                       XK_e,      spawn,          {.v = filemgrcmd } },
	{ MODKEY|ShiftMask,             XK_n,      spawn,          {.v = netmgrcmd } },
	// wicd网络管理程序
	{ MODKEY,                       XK_n,      spawn,          SHCMD("exec wicd-client -n") },
	//计算器
	{ MODKEY,                       XK_c,      spawn,          SHCMD("gcalctool") },
	
	{ MODKEY,                       XK_r,      launcher,          {0} },
	// emacs
	{ MODKEY,                       XK_i,      spawn,          SHCMD("emacs") },
	{ 0,                                XK_Print,  spawn,          {.v = screenshotcmd } },
	{ MODKEY,                       XK_Up,     spawn,          {.v = musicplay } },
	{ MODKEY,                       XK_Down,   spawn,          {.v = musicstop } },
	{ MODKEY,                       XK_Right,  spawn,          {.v = musicnext } },
	{ MODKEY,                       XK_Left,   spawn,          {.v = musicback } },
	// 左右箭头键循环切换tag
	{ MODKEY,                       XK_Left,   view_prev_tag,  {0} },
	{ MODKEY,                       XK_Right,  view_next_tag,  {0} },
	{ MODKEY,                       XK_s,      spawn,          {.v = stardict} },
	{ MODKEY|ShiftMask,             XK_s,      spawn,          {.v = musickcrwalt } },
	{ 0,                            0x1008ff13,	spawn,         {.v = volumeup } },
	{ 0,                            0x1008ff11,	spawn,         {.v = volumedown } },
	{ 0,                            0x1008ff12,	spawn,         {.v = volumemute } },
	//打开音量调整界面(末尾加-g参数，则界面为黑白)
	{ MODKEY,                       XK_v,      spawn,          SHCMD("exec xterm -e alsamixer") },
	{ MODKEY,                       XK_F11,	spawn,             {.v = volumeup } },	//音量增加
	{ MODKEY,                       XK_F12,	spawn,             {.v = volumedown } },	//音量减小
	{ MODKEY,                       XK_F10,	spawn,             {.v = volumemute } },	//静音
	{ MODKEY,                       XK_b,      togglebar,      {0} },	//隐藏状态栏
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	//关闭当前正在使用的窗口
	{ MODKEY|ShiftMask,             XK_c,      killclient ,     {0} },		
	{ MODKEY,                       XK_Escape, killclient ,     {0} },
	
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY|ShiftMask,             XK_t,      setlayout,      {.v = &layouts[3]} },  
	{ MODKEY,                       XK_g,      setlayout,      {.v = &layouts[4]} },
	{ MODKEY|ShiftMask,             XK_g,      setlayout,      {.v = &layouts[5]} },
	
	{ MODKEY,                       XK_space,  setlayout,      {0} }, 
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} }, 
	//使所用的窗口集中到一起（在一个屏幕中显示），Super+1等数字还原操作
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } }, 
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },	/* restart dwm on-the-fly */
	{ MODKEY|ControlMask,           XK_q,      spawn,          SHCMD("killall dwm") },	/* kill the dwm session */
	//重启dwm，无需登出dwm
	{ MODKEY|ShiftMask,             XK_r,      restart,        {0} },
		
	//任务管理器
	{ MODKEY ,                      XK_z,      spawn,          SHCMD("xfce4-taskmanager") },	
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)

};

/* 鼠标动作定义 button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[]															= {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,           0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,           0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,           0,              Button2,        zoom,           {0} },
	{ ClkStatusText,       0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,              0,              Button1,        view,           {0} },
	{ ClkTagBar,              0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,              MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,              MODKEY,         Button3,        toggletag,      {0} },

	// 用鼠标滚轮在标签页区域滚动时切换标签页
	//xlib中 鼠标的前滚和后滚被映射到了鼠标的第 4 和第 5 按键上，如果需要进行鼠标滚轮事件的处理，只需要在 ButtonPress 里面进行即可。这点与Windows不同，Windows里面有专门的鼠标滚动处理事件WM_MOUSEWHEEL。
	{ ClkTagBar,            0,              Button4,        view_prev_tag ,           {0} },
	{ ClkTagBar,            0,              Button5,        view_next_tag ,           {0} },
	// 鼠标滚轮在窗口标题显示区域滚动时切换窗口焦点
	{ ClkWinTitle,          0,              Button4,       focusstack,      {.i = -1 } },
	{ ClkWinTitle,          0,              Button5,       focusstack,      {.i = +1 } },
	// 鼠标左键单击窗口标题显示区域，切换窗口
	{ ClkWinTitle,          0,              Button1,       focusonclick,   {0} },
    // 鼠标右键单击窗口标题关闭当前活动窗口
    { ClkWinTitle,          0 ,             Button3 ,      closeonclick ,     {0} },

};

