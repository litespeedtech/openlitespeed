<?php

class RawFiles
{
	private $_list = array();
	// list of obj (name, level, parrent)

	private $_errs = array();
	private $_fatal = 0;

	public function GetFileName($fid)
	{
		return $this->_list[$fid][0];
	}

	public function AddRawFile($node)
	{
		$filename = $node->Get(CNode::FLD_VAL);
		$index = count($this->_list);
		$level = ($index > 0) ? $this->_list[$parent][1] + 1 : 0;

		$this->_list[$index] = array($filename, $level, $index);

		return $index;
	}

	public function MarkError($node, $errlevel, $errmsg)
	{
		$node->SetErr($errmsg, $errlevel);
		$this->_errs[] = array($errlevel, $errmsg, $node->Get(CNode::FLD_FID), $node->Get(CNode::FLD_FLFROM), $node->Get(CNode::FLD_FLTO));
		if ($errlevel == CNode::E_FATAL)
			$this->_fatal ++;
	}

	public function HasFatalErr()
	{
		return ($this->_fatal > 0);
	}

	public function HasErr()
	{
		return (count($this->_errs) > 0);
	}

	public function GetAllErrors()
	{
		$level = array(CNode::E_WARN => 'WARN', CNode::E_FATAL => 'FATEL');

		$buf = "\nShow Errors: \n";
		foreach ($this->errs as $e) {
			$errlevel = $level[$e[0]];
			$filename = $_list[$e[2]]->filename;
			$buf .= "$errlevel $filename line $e[3]";
			if ($e[3] != $e[4])
				$buf .= " ~ $e[4]";
			$buf .= ": $e[1]\n";
		}
		return $buf;
	}
}


class PlainConfParser
{

	public function __construct()
	{}

	public function Parse($filename)
	{
		$parser = new PlainConfParser();
		$root = new CNode(CNode::K_ROOT, $filename, CNode::T_ROOT);
		$rawfiles = new RawFiles();

		$parser->parse_raw($rawfiles, $root);

		return $root;
	}

	private function parse_raw($rawfiles, $root)
	{
		$fid = $rawfiles->AddRawFile($root);

		$filename = $root->Get(CNode::FLD_VAL);
		$rawlines = file($filename);

		if ($rawlines == NULL) {
			$errlevel = ($root->Get(CNode::FLD_KEY) == CNode::K_ROOT) ? CNode::E_FATAL : CNode::E_WARN;
			$errmsg = "Failed to read file $filename";
			$rawfiles->MarkError($root, $errlevel, $errmsg);
			return;
		}

		$root->SetRawMap($fid, 1, count($rawlines), '');

		$stack = array();
		$cur_node = $root;
		$prev_node = NULL;
		$cur_val = '';
		$cur_comment = '';
		$from_line = 0;
		$to_line = 0;

		$sticky = FALSE;
		$multiline_tag = '';

		foreach ($rawlines as $line_num => $data) {

			$line_num ++;

			if ($sticky || ($multiline_tag != ''))
				$d = rtrim($data, "\r\n");
			else
				$d = trim($data);

			if ($d == '') {
				$cur_comment .= "\n";
				continue; 	// ignore empty lines
			}

			if ($d[0] == '#') {
				$cur_comment .= $d . "\n";
				continue;	// comments
			}

			if (!$sticky && $multiline_tag == '') {
				$from_line = $line_num;
			}

			$end_char = $d[strlen($d) - 1];

			$cur_val .= $d;

			if ($end_char == '\\') {
				$sticky = TRUE;
				$cur_val .= "\n"; //make the line end with \n\
				continue;
			}
			else
				$sticky = FALSE;

			if ($multiline_tag != '') {
				if (trim($d) == $multiline_tag) // stop
					$multiline_tag = '';
				else {
					$cur_val .= "\n";
					continue;
				}
			}
			elseif (($pos = strpos($d, '<<<')) > 0 ) {
				$multiline_tag = trim(substr($d, $pos+3));
				$cur_val .= "\n";
				continue;
			}

			$to_line = $line_num;

			if ($d[0] == '}') {
				// end of block

				if ($cur_comment != '') {
					$cur_node->AddEndComment($cur_comment);
				}

				if (strlen($cur_val) > 1) {
					$rawfiles->MarkError($cur_node, CNode::E_WARN, 'No other characters allowed at the end of closing }');
				}

				if (count($stack) > 0) {
					$prev_node = $cur_node;
					$prev_node->Set(CNode::FLD_FLTO, $line_num);
					$cur_node = array_pop($stack);
				}
				else {
					$rawfiles->MarkError(($prev_node == NULL)? $cur_node : $prev_node, CNode::E_FATAL,
							'Mismatched blocks, may due to extra closing }');
				}
			}
			else {

				$is_block = FALSE;
				if ($end_char == '{') {
					$cur_val = rtrim(substr($cur_val, 0, (strlen($cur_val) - 1)));
					$is_block = TRUE;
				}

				if (preg_match('/^([\S]+)\s/', $cur_val, $m)) {
					$key = $m[1];
					$val = trim(substr($cur_val, strlen($m[0])));
					if (substr($val, 0, 3) == '<<<') {
						$posv0 = strpos($val, "\n");
						$posv1 = strrpos($val, "\n");
						$val = trim(substr($val, $posv0 + 1, $posv1-$posv0));
					}
				}
				else {
					$key = $cur_val;
					$val = NULL;
				}

				$type = CNode::T_KV;
				if ($is_block)
					$type = ($val == NULL) ? CNode::T_KB : CNode::T_KVB;
				elseif (strcasecmp($key, 'include') == 0)
					$type = CNode::T_INC;


				$newnode = new CNode($key, $val, $type);
				$newnode->SetRawMap($fid, $from_line, $to_line, $cur_comment);
				// validate key
				if (!preg_match('/^([a-zA-Z_0-9:])+$/', $key))
					$rawfiles->MarkError($newnode, CNode::E_WARN, "Invalid char in keyword $key");

				$cur_node->AddChild($newnode);

				if ($newnode->HasFlag(CNode::BM_BLK)) {
					$stack[] = $cur_node;
					$prev_node = $cur_node;
					$cur_node = $newnode;
				}
				elseif ($newnode->HasFlag(CNode::BM_INC)) {
					$this->parse_raw($rawfiles, $newnode);
				}
			}

			$cur_val = '';
			$cur_comment = '';

		}

		if ($cur_comment != '') {
			$cur_node->AddEndComment($cur_comment);
			$cur_comment = '';
		}
		while (count($stack) > 0) {
			$rawfiles->MarkError($cur_node, CNode::E_FATAL, 'Mismatched blocks at end of the file, may due to extra openning { or missing closing }.');

			$prev_node = $cur_node;
			$cur_node = array_pop($stack);
		}
	}


	public function Test()
	{
		/*
		ini_set('include_path',	'.:ws/');

		date_default_timezone_set('America/New_York');

		spl_autoload_register( function ($class) {
			include $class . '.php';
		});

			$filename = '/home/lsong/proj/temp/test.conf';
			$confdata = new ConfData('serv', $filename);
			echo "Test file $filename \n";
			$root = $this->LoadData($confdata);
			//$this->ExportData($root);


 * 		$parser->_tblDef = DTblDef::GetInstance();
		$filemap = DPageDef::GetInstance()->GetFileMap($confdata->_type);   // serv, vh, tp, admin



			$root->PrintBuf($buf1);
			echo "=======buf1====\n$buf1";

			$exproot = $root->DupHolder();
			$filemap->Convert($root, $exproot, 1, 1);

			$exproot->PrintBuf($buf2);
			echo "=======buf2====\n$buf2";

			$newxml = $exproot->DupHolder();
			$filemap->Convert($exproot, $newxml, 1, 0);
			$newxml->PrintXmlBuf($buf3);
			echo "=======buf3====\n$buf3";

			//$exproot->MergeUnknown($root);
			//$exproot->PrintBuf($buf2);
			//echo $buf2;

			return $root;*/

	}

}

$parser = new PlainConfParser();
$parser->Test();
