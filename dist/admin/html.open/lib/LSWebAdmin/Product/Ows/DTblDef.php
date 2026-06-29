<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\WebServer\Ols\DTblDefBase as OlsDTblDefBase;
use LSWebAdmin\UI\DTbl;

class DTblDef extends OlsDTblDefBase
{
	protected function add_T_GENERAL2($id)
	{
		$attrs = [
			$this->_attrs['tp_vrFile']->dup('docRoot', DMsg::ALbl('l_docroot'), 'templateVHDocRoot'),
			$this->_attrs['adminEmails']->dup(null, null, 'vhadminEmails'),
			$this->_attrs['vh_enableGzip'],
			$this->_attrs['vh_enableBr'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['vh_cgroups'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_vhostconfigdefaults'), $attrs, 'templateVHConfigDefaults');
	}

	protected function add_T_REALM_TOP($id)
	{
		$align = ['center', 'center', 'center'];
		$realm_attr = $this->get_realm_attrs();

		$attrs = [
			$realm_attr['realm_name'],
			self::NewViewAttr('userDB:location', DMsg::ALbl('l_userdblocation'), 'userDBLocation'),
			self::NewActionAttr('T_REALM_FILE', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_realmlist'), $attrs, 'name', 'T_REALM_FILE', $align, 'realms', 'shield', TRUE);
	}

}
