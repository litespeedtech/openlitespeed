<?php

// To customize look & feel of generated index page
class UserSettings
{

	public $HeaderName = 'HEADER';
	public $ReadmeName = 'README';
	public $Exclude_Patterns = ['.',
		'..',
		'.??*',
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
	public $Time_Format = ' d-M-Y H:i ';
	public $IconPath = '/_autoindex/icons';
	public $nameWidth = 80;
	public $nameFormat;

	public function __construct()
	{
		$this->nameFormat = '%-' . ($this->nameWidth + 4) . '.' . ($this->nameWidth + 4) . 's';
	}

}

class IMG_Mapping
{

	public $suffixes;
	public $imageName;
	public $desc;
	public $alt;

	public function __construct($arrSuffix, $imgName, $altName, $descr='')
	{
		$this->suffixes = $arrSuffix;
		$this->imageName = $imgName;
		$this->alt = $altName;
		$this->desc = $descr;
	}

	public function found($ext)
	{
		return in_array($ext, $this->suffixes);
	}

}

class AllImgs
{

	public $mapping, $default_img, $dir_img;

	public function __construct()
	{
		$this->mapping = [
			new IMG_Mapping(['gif', 'png', 'jpg', 'jpeg', 'tif', 'tiff', 'bmp', 'svg', 'raw'],
					'image.png', '[IMG]'),
			new IMG_Mapping(['html', 'htm', 'shtml', 'php', 'phtml', 'css', 'js'],
					'html.png', '[HTM]'),
			new IMG_Mapping(['txt', 'md5', 'c', 'cpp', 'cc', 'h', 'sh'],
					'text.png', '[TXT]'),
			new IMG_Mapping(['gz', 'tgz', 'zip', 'Z', 'z'],
					'compress.png', '[CMP]'),
			new IMG_Mapping(['bin', 'exe'],
					'binary.png', '[BIN]'),
			new IMG_Mapping(['mpg', 'avi', 'mpeg', 'ram', 'wmv'],
					'movie.png', '[VID]'),
			new IMG_Mapping(['mp3', 'mp2', 'ogg', 'wav', 'wma', 'aac', 'mp4', 'rm'],
					'sound.png', '[SND]'),
		];

		$this->default_img = new IMG_Mapping(null, 'unknown.png', 'unknown', '');
		$this->dir_img = new IMG_Mapping(null, 'folder.png', 'directory', '');
		$this->parent_img = new IMG_Mapping(null, 'up.png', 'up', '');
	}

	public function findImgMapping($file)
	{
		$found = null;
		$pos = strrpos($file, '.');
		if ($pos !== false) {
			$ext = substr($file, $pos + 1);
			if ($ext !== false) {
				$l = count($this->mapping);
				for ($i = 0; $i < $l; ++$i) {
					if (in_array($ext, $this->mapping[$i]->suffixes)) {
						$found = $this->mapping[$i];
						break;
					}
				}
			}
		}
		if (!isset($found))
			$found = $this->default_img;
		return $found;
	}

}

// END of customization section



class FileStat
{

	public $name;
	public $size;
	public $mtime;
	public $isdir;
	public $img;

	public function __construct($filename)
	{
		$this->name = $filename;
	}

}

function shouldExclude($file, &$excludes)
{
	$ex = reset($excludes);
	foreach ($excludes as $ex) {
		if (fnmatch($ex, $file))
			return true;
	}
	return false;
}

function readDirList($path, &$excludes, &$map)
{
	$handle = opendir($path);
	if ($handle === false) {
		return null;
	}
	clearstatcache();
	$list = [];
	if (isset($_SERVER['LS_AI_INDEX_IGNORE'])) {
		$ignore = explode(' ', $_SERVER['LS_AI_INDEX_IGNORE']);
		$excludes = array_merge($ignore, $excludes);
	}
	while (false !== ($file = readdir($handle))) {
		if (shouldExclude($file, $excludes)) {
			continue;
		}
		$fileStat = new FileStat($file);
		$s = stat("$path$file");
		$fileStat->mtime = $s[9];
		$fileStat->isdir = ($s[2] & 040000) ? '/' : '';

		// get image mapping
		if ($fileStat->isdir) {
			$fileStat->size = -1;
			$fileStat->img = $map->dir_img;
		} else {
			if ($s[12] > 0)
				$fileStat->size = 512 * $s[12];
			else
				$fileStat->size = $s[7];
			$fileStat->img = $map->findImgMapping($file);
		}

		$list[] = $fileStat;
	}
	closedir($handle);
	return $list;
}

function printOneEntry($base, $name, $fileStat, $setting)
{
	$encoded = str_replace(['%2F', '%26amp%3B'], ['/', '%26'],
			rawurlencode($base . $fileStat->name));
	if (isset($_SERVER['LS_FI_OFF']) && $_SERVER['LS_FI_OFF']) {
		$buf = '<li>' . '<a href="' . $encoded .
				$fileStat->isdir . '">' . sprintf($setting->nameFormat, htmlspecialchars($name, ENT_SUBSTITUTE) . "</a></li>\n");
	} else {
		$buf = '<img src="' . $setting->IconPath . '/' . $fileStat->img->imageName .
				'" alt="' . $fileStat->img->alt . '"> <a href="' . $encoded . $fileStat->isdir . '">';
		if (strlen($name) > $setting->nameWidth) {
			$name = substr($name, 0, $setting->nameWidth - 3) . '...';
		}
		$buf .= sprintf($setting->nameFormat, htmlspecialchars($name, ENT_SUBSTITUTE) . "</a>");
		if ($fileStat->mtime != -1)
			$buf .= date($setting->Time_Format, $fileStat->mtime);
		else
			$buf .= '                   ';
		if ($fileStat->size != -1)
			$buf .= sprintf("%7ldk  ", ( $fileStat->size + 1023 ) / 1024);
		else
			$buf .= '       -  ';
		$buf .= '     ' . $fileStat->img->desc;
		$buf .= "\n";
	}
	echo $buf;
}

function printIncludes($path, $name)
{
	$testNames = ["$name.html", "$name.htm", $name];
	foreach ($testNames as $n) {
		$filename = $path . $n;

		if (file_exists($filename) && !is_link($filename)) {
			$content = file_get_contents($filename);
			if ($n == $name) {
				echo "<pre>\n";
				echo $content;
				echo "</pre>\n";
			} else { // html format
				echo $content;
			}
			break;
		}
	}
}

function printFileList($list, $base_uri, $setting)
{
	foreach ($list as $fileStat) {
		if ($fileStat->isdir) {
			printOneEntry($base_uri, $fileStat->name, $fileStat, $setting);
		}
	}

	foreach ($list as $fileStat) {
		if (!$fileStat->isdir) {
			printOneEntry($base_uri, $fileStat->name, $fileStat, $setting);
		}
	}
}

function cmpNA($a, $b)
{
	return strcmp($a->name, $b->name);
}

function cmpND($a, $b)
{
	return -cmpNA($a, $b);
}

function cmpSA($a, $b)
{
	$ret = $a->size - $b->size;
	if ($ret)
		return $ret;
	return cmpNA($a, $b);
}

function cmpSD($a, $b)
{
	return -cmpSA($a, $b);
}

function cmpMA($a, $b)
{
	return $a->mtime - $b->mtime;
}

function cmpMD($a, $b)
{
	return -cmpMA($a, $b);
}

function cmpDA($a, $b)
{
	$ret = strcmp($a->img->desc, $b->img->desc);
	if ($ret)
		return $ret;
	return strcmp($a->name, $b->name);
}

function cmpDD($a, $b)
{
	return -cmpDA($a, $b);
}

ini_set('open_basedir', $_SERVER['DOCUMENT_ROOT']);

$pos = strpos($_SERVER['REQUEST_URI'], '?');
if ($pos === false) {
	$uri = $_SERVER['REQUEST_URI'];
} else {
	$uri = substr($_SERVER['REQUEST_URI'], 0, $pos);
}

$uri = urldecode($uri);
$path = $_SERVER['LS_AI_PATH'];

$mime_type = $_SERVER['LS_AI_MIME_TYPE'];
if ($mime_type) {
	header("Content-Type: $mime_type");
}

if (!$path) {
	echo "[ERROR] Auto Index script can not be accessed directly!";
	exit;
}


$uri = htmlentities($uri, ENT_COMPAT, 'UTF-8');

$setting = new UserSettings();
$map = new AllImgs();
$sortOrder = $_SERVER['QUERY_STRING'];
if ($sortOrder == '' || strlen($sortOrder) != 2 || !in_array($sortOrder, ['NA', 'ND', 'MA', 'MD', 'SA', 'SD', 'DA', 'DD'])) {
	$sortOrder = 'NA'; // set to default
}

$NameSort = ($sortOrder == 'NA') ? 'ND' : 'NA';
$ModSort = ($sortOrder == 'MA') ? 'MD' : 'MA';
$SizeSort = ($sortOrder == 'SA') ? 'SD' : 'SA';
$DescSort = ($sortOrder == 'DA') ? 'DD' : 'DA';

$list = readDirList($path, $setting->Exclude_Patterns, $map);
if ($list === null) {
	http_response_code(403);
	echo '<h1>403 Access Denied</h1>';
	exit;
}

$using_fancyIndex = !empty($_SERVER['LS_FI_OFF']);

echo "<!DOCTYPE html>
<html>
  <head>
  <meta http-equiv=\"Content-type\" content=\"text/html; charset=UTF-8\" />
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />
  <title>Index of ", $uri, "</title></head>
  <body>
    <h1>Index of ", $uri, "</h1>";

if (isset($setting->HeaderName)) {
	printIncludes($path, $setting->HeaderName);
}

if ($using_fancyIndex) {
	$header = "<ul>\n";
} else {
	$header = "<pre><img src=\"$setting->IconPath/blank.png\" alt=\"     \"> <a href='?$NameSort'>";
	$header .= sprintf($setting->nameFormat, 'Name</a>');
	$header .= " <a href='?$ModSort'>Last modified</a>         <a href='?$SizeSort'>Size</a>  <a href='?$DescSort'>Description</a>\n   <hr>";
}
echo $header;

if ($uri != '/') {
	$fileStat = new FileStat('');
	$fileStat->mtime = filemtime($path);
	$fileStat->img = $map->parent_img;
	$fileStat->size = -1;
	$base = substr($uri, 0, strlen($uri) - 1);
	$off = strrpos($base, '/');
	if ($off !== false) {
		$base = substr($base, 0, $off + 1);
		printOneEntry($base, 'Parent Directory', $fileStat, $setting);
	}
}
$cmpFunc = "cmp$sortOrder";
usort($list, $cmpFunc);
printFileList($list, $uri, $setting);

if ($using_fancyIndex) {
	echo "</ul>\n";
} else {
	echo "</pre><hr>";
}

if (isset($setting->ReadmeName)) {
	printIncludes($path, $setting->ReadmeName);
}

echo '<address>Proudly Served by LiteSpeed Web Server at ' . $_SERVER['SERVER_NAME'] . ' Port ' . $_SERVER['SERVER_PORT'] . "</address>
</body>
</html>";
