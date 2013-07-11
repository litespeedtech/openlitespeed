<?

class XUI {

	
	public static function message($title="", $msg="", $type = "normal") {

		return "<div class=panel_{$type} align=left><span class='gui_{$type}'>" . (strlen($title) ? "$title<hr size=1 noshade>" : "") . "$msg</span></div>";


	}
}

?>