<?php

class DPage
{
	var $_type;//serv, vh, sl, admin
	var $_id;
	var $_name;
	var $_title;
	var $_helpLink;
	var $_tblIds;
	var $_includeScript;

	public function __construct($type, $id, $name, $title, $tblIds)
	{
		$this->_type = $type;
		$this->_id = $id;
		$this->_name = $name;
		$this->_title = $title;
		$this->_tblIds = $tblIds;
		$this->_helpLink = 'index.html';
	}

	public function PrintHtml(&$confData, $disp)
	{


		if ( $disp->_act == 'd' || $disp->_act == 'i' )
			$this->printActionConfirm($disp);

		if ( $disp->_err != NULL )
		{
			echo XUI::message("",$disp->_err,"error");
		}

		$tblDef = DTblDef::GetInstance();
		if ( $disp->_tid == NULL )
		{
			$tids = &$this->_tblIds;
		}
		else
		{
			$tid = DUtil::getLastId($disp->_tid);
			$tids = array($tid);
		}
		$ref = DUtil::getLastId($disp->_ref);

		foreach ($tids as $ti )
		{
			$tbl = $tblDef->GetTblDef($ti);
			$isExtract = false;

			if ( $disp->_act == 'a' ) {
				$d = array();
			}
			else if ( $disp->_act == 'S'
					  || $disp->_act == 'c'
					  || $disp->_act == 'n') //save failed or in change
			{
				$isExtract = true;
				$d = &$confData;
			}
			else
			{
				$d = DUtil::locateData($confData, $tbl->_dataLoc, $disp->_ref);
			}

			if ( $tbl->_holderIndex != NULL )
			{
				if ( $disp->_act == 'e' || $disp->_act == 'E' || $disp->_act == 'a')
				{
					$disp->_info['holderIndex'] = is_array($d)? array_keys($d):NULL;
					$disp->_info['holderIndex_cur'] = $ref;
				}
				if ( !$isExtract && $ref != NULL )
					$d = &$d[$ref];
			}
			else
				$disp->_info['holderIndex'] = NULL;


			echo "<table width=100%><tr><td>";
			$tbl->PrintHtml($d, $ref, $disp);
			echo "</td></tr></table>";

		}


	}

	private function printActionConfirm($disp)
	{
		$hasB = (strpos($disp->_tid, '`') !== false );
		echo '<div class=message>';
		if ( $disp->_act == 'd' )
		{
			$actString = $hasB ? 'DC' : 'Dc';
			//should validate whether there is reference to this entry
			echo 'Are you sure you want to delete this entry?<br>'.
				'<span class="edit-link">'.
				DTbl::getActionLink($disp, $actString) . '</span>'.
				'<br>This will be permanently removed from the configuration file.';
		}
		else if ( $disp->_act == 'i' )
		{
			$actString = $hasB ? 'IC' : 'Ic';
			echo 'Are you sure you want to instantiate this virtual host?<br>'.
				'<span class="edit-link">'.
				DTbl::getActionLink($disp, $actString) . '</span>'.
				'<br>This will create a dedicated configuration file for this virtual host.';
		}

		echo "</div>";

	}
}

