
		void append_page_header() {
		webpage = ("<!DOCTYPE html><html>");
		webpage += ("<head>");
		webpage += ("<title>File Server</title>"); // NOTE: 1em = 16px
		webpage += ("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
		webpage += ("<style>");
		webpage += ("body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}");
		webpage += ("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}");
		webpage += ("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
		webpage += ("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}");
		webpage += ("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
		webpage += ("section {font-size:0.88em;}");
		webpage += ("h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}");
		webpage += ("h2{color:orange;font-size:1.0em;}");
		webpage += ("h3{font-size:0.8em;}");
		webpage += ("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}");
		webpage += ("th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;}");
		webpage += ("tr:nth-child(odd) {background-color:#eeeeee;}");
		webpage += (".rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}");
		webpage += (".rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}");
		webpage += (".rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}");
		webpage += (".column{float:left;width:50%;height:45%;}");
		webpage += (".row:after{content:'';display:table;clear:both;}");
		webpage += ("*{box-sizing:border-box;}");
		webpage += ("footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
		webpage += ("button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}");
		webpage += (".buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}");
		webpage += (".buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}");
		webpage += (".buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}");
		webpage += ("a{font-size:75%;}");
		webpage += ("p{font-size:75%;}");
		webpage += ("</style></head><body><h1>File Server "); webpage += String(ServerVersion) + "</h1>";
	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	void append_page_footer() { // Saves repeating many lines of code for HTML page footers
		webpage += ("<ul>");
		webpage += ("<li><a href='/'>Home</a></li>"); // Lower Menu bar command entries
		webpage += ("<li><a href='/download'>Download</a></li>");
		webpage += ("</ul>");
		webpage += "<footer>&copy;" + String(char(byte(0x40 >> 1))) + String(char(byte(0x88 >> 1))) + String(char(byte(0x5c >> 1))) + String(char(byte(0x98 >> 1))) + String(char(byte(0x5c >> 1)));
		webpage += String(char((0x84 >> 1))) + String(char(byte(0xd2 >> 1))) + String(char(0xe4 >> 1)) + String(char(0xc8 >> 1)) + String(char(byte(0x40 >> 1)));
		webpage += String(char(byte(0x64 / 2))) + String(char(byte(0x60 >> 1))) + String(char(byte(0x62 >> 1))) + String(char(0x70 >> 1)) + "</footer>";
		webpage += ("</body></html>");
	};
