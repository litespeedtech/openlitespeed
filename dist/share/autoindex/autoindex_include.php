<?php

namespace LiteSpeedAutoIndex;

class UserSettings
{

	public static $TIME_FORMAT = 'Y-m-d H:i';

	/**
	 * If file name is longer than this widtch, will cutoff and append with ...
	 */
	public static $NAME_WIDTH = 80;

	/**
	 *
	 * For JS version, if the file list has more than this limit, we will show the name filter
	 */
	public static $FILTER_SHOW = 6;

	/**
	 *
	 * @return string if you return empty string or null, will not check and include any Header
	 */
	public static function getHeaderName()
	{
		return 'HEADER';
	}

	/**
	 *
	 * @return string if you return empty string or null, will not check and include any Readme
	 */
	public static function getReadmeName()
	{
		return 'README';
	}

	public static function getExcludePatterns()
	{
		return ['.',
			'..',
			'.?*',
			'*~',
			'*#',
			'HEADER*',
			'README*',
			'RCS',
			'CVS',
			'*,v',
			'*,t',
			'*.lsz',
		];
	}

	private static $exclude_list;

	public static function shouldExclude($filename)
	{
		if (self::$exclude_list == null) {
			self::$exclude_list = self::getExcludePatterns();
			if (!empty($_SERVER['LS_AI_INDEX_IGNORE'])) {
				self::$exclude_list = array_merge(explode(' ', $_SERVER['LS_AI_INDEX_IGNORE']), self::$exclude_list);
			}
		}

		reset(self::$exclude_list);
		foreach (self::$exclude_list as $ex) {
			if (fnmatch($ex, $filename))
				return true;
		}
		return false;
	}

}

class IconMap
{

	private $icons;
	private $definedSuffix;
	private static $instance;

	private function __construct()
	{
		$this->icons = [
			'DEFAULT' => ['file.svg', 'File'],
			'UP' => ['corner-left-up.svg', 'Up'],
			'DIR' => ['folder-fill.svg', 'Directory'],
			'IMG' => ['image.svg', '[IMG]'],
			'TXT' => ['file-text.svg', '[TXT]'],
			'CMP' => ['file.svg', '[CMP]'],
			'BIN' => ['file.svg', '[BIN]'],
			'VID' => ['video.svg', '[VID]'],
			'SND' => ['music.svg', '[SND]'],
		];

		$this->definedSuffix = [];
		$this->addMapping(['gif', 'png', 'jpg', 'jpeg', 'webp', 'tif', 'tiff', 'bmp', 'svg', 'raw'],
				'IMG');
		$this->addMapping(['txt', 'md5', 'c', 'cpp', 'cc', 'h', 'sh', 'html', 'htm', 'shtml', 'php', 'phtml', 'css', 'js'],
				'TXT');
		$this->addMapping(['gz', 'tgz', 'zip', 'Z', 'z'], 'CMP');
		$this->addMapping(['bin', 'exe'], 'BIN');
		$this->addMapping(['mpg', 'avi', 'mpeg', 'ram', 'wmv'], 'VID');
		$this->addMapping(['mp3', 'mp2', 'ogg', 'wav', 'wma', 'aac', 'mp4', 'rm'], 'SND');
	}

	private function addMapping($suffixArray, $iconTag)
	{
		if (!isset($this->icons[$iconTag])) { // should not happen, unrecognized tag
			$iconTag = 'DEFAULT';
		}
		foreach ($suffixArray as $suffix) {
			$this->definedSuffix[$suffix] = $iconTag;
		}
	}

	private static function getMap()
	{
		if (self::$instance == null) {
			self::$instance = new self();
		}
		return self::$instance;
	}

	public static function getFileIconTag($file)
	{
		$map = self::getMap();
		$pos = strrpos($file, '.');
		if ($pos !== false) {
			$suffix = substr($file, $pos + 1);
			if ($suffix !== false) {
				if (isset($map->definedSuffix[$suffix])) {
					return $map->definedSuffix[$suffix];
				}
			}
		}
		return 'DEFAULT';
	}

	public static function img($iconTag)
	{
		$me = self::getMap();
		return '<img class="icon" src="/_autoindex/assets/icons/'
				. $me->icons[$iconTag][0] . '" alt="' . $me->icons[$iconTag][1] . '">';
	}

}

// END of customization section


class FileStat
{

	protected $name;
	protected $size;
	protected $mtime;
	protected $isdir;
	protected $iconTag;
	protected $restricted = false;

	public function __construct($path, $filename)
	{
		$this->name = $filename;

		if ($path == '..') {
			$this->size = -2;
			$this->iconTag = 'UP';
			$this->mtime = 0;
			return;
		}

		$s = @stat($path . $filename);
		if ($s == false) {
			$this->restricted = true;
			return;
		}
		$this->mtime = $s[9];

		if (($s[2] & 040000) > 0) {
			$this->isdir = true;
			$this->size = -1;
			$this->iconTag = 'DIR';
		} else {
			$this->isdir = false;
			$this->size = ($s[12] > 0) ? (512 * $s[12]) : $s[7];
			$this->iconTag = IconMap::getFileIconTag($filename);
		}
	}

	public function isRestricted()
	{
		// due to openbase_dir, file is outside of DOCROOT
		return $this->restricted;
	}

	public function isDir()
	{
		return $this->isdir;
	}

	public function getUrl($base)
	{
		$encoded = $base . rawurlencode($this->name);
		if ($this->isdir) {
			$encoded .= '/';
		}
		return $encoded;
	}

	public function getIconTag()
	{
		return $this->iconTag;
	}

	public function sortName()
	{
		// case insensitive compare
		$name = htmlspecialchars(strtolower($this->name), ENT_QUOTES | ENT_SUBSTITUTE);
		return $this->isdir ? '*' . $name : $name; // to make directories list on top
	}

	public function dispName()
	{
		$name = $this->name;
		if (strlen($name) > UserSettings::$NAME_WIDTH) {
			$name = substr($name, 0, UserSettings::$NAME_WIDTH - 3) . '...';
		}
		return htmlspecialchars($name, ENT_QUOTES | ENT_SUBSTITUTE);
	}

	public function dispTime()
	{
		return ($this->mtime > 0) ? date(UserSettings::$TIME_FORMAT, $this->mtime) : '';
	}

	public function sortTime()
	{
		if ($this->isdir) {
			// to make dir sorted together, deduct 50 years
			return ($this->mtime - 1576800000);
		}
		return $this->mtime;
	}

	public function dispSize()
	{
		if ($this->iconTag == 'UP') { // parent dir
			return '';
		}
		if ($this->isdir) {
			return '-';
		}
		return sprintf("%7ldk", ($this->size + 1023) / 1024);
	}

	public function sortSize()
	{
		return $this->size;
	}

	public static function cmpNA($a, $b)
	{
		return strcasecmp($a->name, $b->name);
	}

	public static function cmpND($a, $b)
	{
		return - self::cmpNA($a, $b);
	}

	public static function cmpSA($a, $b)
	{
		$ret = $a->size - $b->size;
		if ($ret == 0) {
			// if same size, compare by name ascending
			return self::cmpNA($a, $b);
		}
		return $ret;
	}

	public static function cmpSD($a, $b)
	{
		return - self::cmpSA($a, $b);
	}

	public static function cmpMA($a, $b)
	{
		return $a->mtime - $b->mtime;
	}

	public static function cmpMD($a, $b)
	{
		return - self::cmpMA($a, $b);
	}

}

class Directory
{

	private $list;
	private $path;
	private $len;

	public function __construct($path)
	{
		$this->path = $path;
		$handle = opendir($path);
		if ($handle === false) {
			return;
		}
		clearstatcache();
		$this->list = [];
		while (false !== ($file = readdir($handle))) {
			if (!UserSettings::shouldExclude($file)) {
				$fs = new FileStat($path, $file);
				if (!$fs->isRestricted()) {
					$this->list[] = $fs;
				}
			}
		}
		closedir($handle);
		$this->len = count($this->list);
	}

	public function cannotLoad()
	{
		return ($this->list === null);
	}

	public function getListCount()
	{
		return $this->len;
	}

	public function sortList($order)
	{
		usort($this->list, ['\\LiteSpeedAutoIndex\\FileStat', "cmp$order"]);
	}

	public function populateList(&$dirs, &$files)
	{
		$files = [];
		foreach ($this->list as $fileStat) {
			if ($fileStat->isDir()) {
				$dirs[] = $fileStat;
			} else {
				$files[] = $fileStat;
			}
		}
	}

}

class Index
{

	protected $dir;
	protected $uri;
	protected $path;
	protected $isFancy;
	protected $sort;

	public function printPage()
	{
		$this->init();
		$this->printHeader();
		$this->printContent();
		$this->printFooter();
	}

	protected function init()
	{
		if (empty($_SERVER['LS_AI_PATH'])) {
			$this->exit403('[ERROR] Auto Index script can not be accessed directly!');
		}
		$this->path = $_SERVER['LS_AI_PATH'];

		ini_set('open_basedir', $_SERVER['DOCUMENT_ROOT']);

		$this->dir = new Directory($this->path);
		if ($this->dir->cannotLoad()) {
			$this->exit403('<h1>403 Access Denied</h1>');
		}

		$uri = $_SERVER['REQUEST_URI'];
		if (($pos = strpos($uri, '?')) !== false) {
			$uri = substr($_SERVER['REQUEST_URI'], 0, $pos);
		}

		$this->uri = $uri;

		if (isset($_SERVER['LS_AI_MIME_TYPE'])) {
			header('Content-Type: ' . $_SERVER['LS_AI_MIME_TYPE']);
		}

		$this->isFancy = empty($_SERVER['LS_FI_OFF']); // if not set Fancy Index Off, by default it's ON

		if ($this->isFancy) {
			$this->initSort();
		}
	}

	protected function initSort()
	{
		$order = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '';
		if ($order == '' || strlen($order) != 2 || !in_array($order, ['NA', 'ND', 'MA', 'MD', 'SA', 'SD'])) {
			$order = 'NA'; // set to default
		}
		$this->sort = [];
		$a = 'ascending';
		$d = 'descending';

		switch ($order) {
			case 'NA': $this->sort['name_aria'] = $a; // for indicator for current active sort order.
				$this->sort['N_link'] = 'ND'; // for current header click action link, reverse to current order
				break;
			case 'ND': $this->sort['name_aria'] = $d;
				break;
			case 'MA': $this->sort['mod_aria'] = $a;
				$this->sort['M_link'] = 'MD';
				break;
			case 'MD': $this->sort['mod_aria'] = $d;
				break;
			case 'SA': $this->sort['size_aria'] = $a;
				$this->sort['S_link'] = 'SD';
				break;
			case 'SD': $this->sort['size_aria'] = $d;
				break;
		}
		$this->dir->sortList($order);
	}

	protected function exit403($msg)
	{
		http_response_code(403);
		echo $msg;
		exit;
	}

	protected function getAssetLinks()
	{
		return '<link rel="stylesheet" href="/_autoindex/assets/css/autoindex.css" />';
	}

	protected function getEndBodyScripts()
	{
		return '';
	}

	protected function printHeader()
	{
		$disp_uri = htmlentities(urldecode($this->uri), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');

		$title = 'Index of ' . $disp_uri;

		echo '<!DOCTYPE html><html><head><meta http-equiv="Content-type" content="text/html; charset=UTF-8" /><meta name="viewport" content="width=device-width, initial-scale=1.0" />'
		. $this->getAssetLinks() . '<title>' . $title . "</title></head>\n"
		. '<body><div class="content"><h1>' . $title . "</h1>\n";

		$includeHeader = UserSettings::getHeaderName();
		if ($includeHeader) {
			$this->printIncludes($this->path, $includeHeader);
		}
	}

	protected function printIncludes($path, $name)
	{
		$testNames = ["$name.html", "$name.htm", $name];
		foreach ($testNames as $testname) {
			$filename = $path . $testname;

			if (file_exists($filename) && !is_link($filename)) {
				$content = file_get_contents($filename);
				if (!$content) {
					break;
				}
				$style = ($name == 'HEADER') ? 'header-text' : 'readme-text';
				echo "<div class=\"${style}\">\n" . htmlspecialchars($content, ENT_QUOTES | ENT_SUBSTITUTE) . "</div>\n";
				break;
			}
		}
	}

	protected function printFooter()
	{
		$includeReadme = UserSettings::getReadmeName();
		if ($includeReadme) {
			$this->printIncludes($this->path, $includeReadme);
		}

		echo '<address>Proudly Served by LiteSpeed Web Server at '
		. $_SERVER['SERVER_NAME'] . ' Port ' . $_SERVER['SERVER_PORT']
		. '</address></div>'
		. $this->getEndBodyScripts()
		. "</body></html>\n";
	}

	protected function printOneEntry($base, $fileStat)
	{
		$url = $fileStat->getUrl($base);
		$buf = '<tr><td><a href="' . $url . '">';
		if ($this->isFancy) {
			$buf .= IconMap::img($fileStat->getIconTag())
					. $fileStat->dispName()
					. '</a></td><td>' . $fileStat->dispTime()
					. '</td><td>' . $fileStat->dispSize() . '</td>';
		} else {
			$buf .= $fileStat->dispName() . '</a></td>';
		}
		$buf .= "</tr>\n";
		echo $buf;
	}

	protected function printContent()
	{
		echo '<div id="table-list"><table id="table-content">';
		if ($this->isFancy) {
			$this->printFancyTableHeader();
		} else {
			$this->printTableHeader();
		}
		$dirs = $files = [];
		$this->dir->populateList($dirs, $files);

		if ($this->uri != '/') {
			$fileStat = new FileStat('..', '');
			$base = substr($this->uri, 0, strlen($this->uri) - 1);
			if (($off = strrpos($base, '/')) !== false) {
				$base = substr($base, 0, $off + 1);
				$this->printParentLine($base, $fileStat);
			}
		}

		foreach ($dirs as $fileStat) {
			$this->printOneEntry($this->uri, $fileStat);
		}

		foreach ($files as $fileStat) {
			$this->printOneEntry($this->uri, $fileStat);
		}


		echo "</table></div>\n";
	}

	protected function ariaClass($type)
	{
		$name = $type . '_aria';
		return isset($this->sort[$name]) ? ' aria-sort="' . $this->sort[$name] . '"' : '';
	}

	protected function headerQs($type)
	{
		$name = $type . '_link';
		return isset($this->sort[$name]) ? $this->sort[$name] : $type . 'A'; // default is ascending
	}

	protected function printFancyTableHeader()
	{
		$th = '<th class="colname"';
		echo '<thead class="t-header"><tr>'
		. $th . $this->ariaClass('name') . '><a class="name" href="?' . $this->headerQs('N') . '">Name</a></th>'
		. $th . $this->ariaClass('mod') . '><a href="?' . $this->headerQs('M') . '">Last Modified</a></th>'
		. $th . $this->ariaClass('size') . '><a href="?' . $this->headerQs('S') . '">Size</a></th>'
		. "</tr></thead>\n";
	}

	protected function printTableHeader()
	{
		echo '<thead class="t-header"><tr>'
				. '<th class="colname">Name</th>'
				. '<th class="colname">Last Modified</th>'
				. '<th class="colname">Size</th>'
				. "</tr></thead>\n";
	}

	protected function printParentLine($base, $fileStat)
	{
		$url = $fileStat->getUrl($base);
		if ($this->isFancy) {
			$buf = '<tr><td><a href="' . $url . '">'
					. IconMap::img('UP') . 'Parent Directory'
					. '</a></td><td></td><td></td></tr>';
		} else {
			$buf = '<tr><td><a href="' . $url . '">' . $fileStat->dispName() . '</a></td></tr>';
		}
		echo $buf . "\n";
	}

}

class IndexWithJS extends Index
{

	protected function getAssetLinks()
	{
		return '<link rel="stylesheet" href="/_autoindex/assets/css/autoindex.css" />'
				. '<script src="/_autoindex/assets/js/tablesort.js"></script>'
				. '<script src="/_autoindex/assets/js/tablesort.number.js"></script>';
	}

	protected function getEndBodyScripts()
	{
		if ($this->dir->getListCount() >= UserSettings::$FILTER_SHOW) {
		return <<<EJS
<script>
	new Tablesort(document.getElementById("table-content"));
	var keywordInput = document.getElementById('filter-keyword');
	document.addEventListener('keyup', filterTable);

	function filterTable(e) {
		if (e.target.id != 'filter-keyword') return;

		var cols = document.querySelectorAll('tbody td:first-child');
		var keyword = keywordInput.value.toLowerCase();
		for (i = 0; i < cols.length; i++) {
			var text = cols[i].textContent.toLowerCase();
			if (text != 'parent directory') {
				cols[i].parentNode.style.display = text.indexOf(keyword) === -1 ? 'none' : 'table-row';
			}
		}
	}
</script>
EJS;
		} else {
			return '<script>new Tablesort(document.getElementById("table-content"));</script>';
		}
	}

	protected function printHeader()
	{
		parent::printHeader();
		if ($this->dir->getListCount() >= UserSettings::$FILTER_SHOW) {
			echo '<div id="table-filter"><input type="text" name="keyword" id="filter-keyword" placeholder="Filter Name"></div>' . "\n";
		}
	}

	protected function printFancyTableHeader()
	{
		$onclick = ' onclick="return false"';
		$sortnum = ' data-sort-method="number"';
		$th = '<th class="colname"';

		echo '<thead class="t-header"><tr>'
		. $th . $this->ariaClass('name') . '><a class="name" href="?' . $this->headerQs('N') . '" ' . $onclick . '">Name</a></th>'
		. $th . $sortnum . $this->ariaClass('mod') . '><a href="?' . $this->headerQs('M') . '" ' . $onclick . '">Last Modified</a></th>'
		. $th . $sortnum . $this->ariaClass('size') . '><a href="?' . $this->headerQs('S') . '" ' . $onclick . '">Size</a></th>'
		. "</tr></thead>\n";
	}

	protected function printOneEntry($base, $fileStat)
	{
		$url = $fileStat->getUrl($base);
		$buf = '<tr>';
		if ($this->isFancy) {
			$buf .= '<td data-sort="' . $fileStat->sortName() . '"><a href="' . $url . '">'
					. IconMap::img($fileStat->getIconTag())
					. $fileStat->dispName()
					. '</a></td><td data-sort="' . $fileStat->sortTime() . '">' . $fileStat->dispTime()
					. '</td><td data-sort="' . $fileStat->sortSize() . '">' . $fileStat->dispSize() . '</td>';
		} else {
			$buf .= '<td><a href="' . $url . '">' . $fileStat->dispName() . '</a></td>';
		}
		$buf .= "</tr>\n";
		echo $buf;
	}

	protected function printParentLine($base, $fileStat)
	{
		$url = $fileStat->getUrl($base);
		if ($this->isFancy) {
			$buf = '<tr data-sort-method="none"><td><a href="' . $url . '">'
					. IconMap::img('UP') . 'Parent Directory'
					. '</a></td><td></td><td></td></tr>';
		} else {
			$buf = '<tr><td><a href="' . $url . '">Parent Directory</a></td></tr>';
		}
		echo $buf . "\n";
	}

}
