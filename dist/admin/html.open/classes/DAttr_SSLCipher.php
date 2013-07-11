<?php

class DAttr_SSLCipher extends DAttr
{
	private function getParsedVals($data)
	{
		// $data is cval	
		if ( isset($data) && $data->HasVal())
			$ciphers = explode(':', $data->GetVal());

		$c = array('HIGH' => array('val' => 0, 'label' => 'HIGH', 'desc' => 'Triple-DES encryption'),
				'MEDIUM' => array('val' => 0, 'label' => 'MEDIUM', 'desc' => '128-bit encryption'),
				'LOW' => array('val' => 0, 'label' => 'LOW', 'desc' => '64 or 56 bit encryption but exclude Export cipher suites'),
				'EXPORT56' => array('val' => 0, 'label' => 'EXPORT56', 'desc' => '56-bit Export encryption'),
				'EXPORT40' => array('val' => 0, 'label' => 'EXPORT40', 'desc' => '40-bit Export encryption'));
		
		if (isset($ciphers)) {
			foreach( $ciphers as $cipher ) {
				if ( isset($c[$cipher]) ) {
					$c[$cipher]['val'] = 1;
				}
				else { // for backword compatibility
					$a = substr($cipher,1);
					if ( isset($c[$a]) ) {
						if ( $cipher{0} == '+' )
							$c[$a]['val'] = 1;
					}
				}
			}
		}
		return $c;
	}
	
	private function showHtmlCheckBox($entry)
	{
		$o = '<img src="/static/images/graphics/';
		if ($entry['val'] == 1)
			$o .= 'checked.gif';
		else 
			$o .= 'unchecked.gif';
		$o .= '" width="12" height="12">&nbsp;&nbsp;&nbsp;'
			. $entry['label'] . '&nbsp;&nbsp; <span class="field_novalue">('
			. $entry['desc'] . ')</span>';
		return $o;
	}
	
	private function showInputCheckBox($entry)
	{
		$o = '<input type="checkbox" name="ck' . $entry['label'] . '" value="' . $entry['label'] . '"';
		if ($entry['val'] == 1)
			$o .= 'checked';
		$o .= '>&nbsp;&nbsp;&nbsp;'
			. $entry['label'] . '&nbsp;&nbsp; <span class="field_novalue">('
			. $entry['desc'] . ')</span>';
		return $o;
	}
	
	
	public function extractPost()
	{
		$ciphers = '';
		
		if (isset($_POST['ckHIGH'])) {
			$ciphers .= 'HIGH:';
		}
		if (isset($_POST['ckMEDIUM'])) {
			$ciphers .= 'MEDIUM:';
		}
		if (isset($_POST['ckLOW'])) {
			$ciphers .= 'LOW:';
		}
		if (isset($_POST['ckEXPORT56'])) {
			$ciphers .= 'EXPORT56:';
		}
		if (isset($_POST['ckEXPORT40'])) {
			$ciphers .= 'EXPORT40:';
		}
		
		if ($ciphers != '')
			$ciphers .= '!aNULL:!MD5:!SSLv2:!eNULL:!EDH';
		
		$cval = new CVal($ciphers, $err);
		
		return $cval;
	}		
		


	public function toHtml(&$data, $refUrl=NULL)
	{
		$c = $this->getParsedVals($data);
		$o = $this->showHtmlCheckBox($c['HIGH']) . '<br/>'
			. $this->showHtmlCheckBox($c['MEDIUM']) . '<br/>'
			. $this->showHtmlCheckBox($c['LOW']) . '<br/>'
			. $this->showHtmlCheckBox($c['EXPORT56']) . '<br/>'
			. $this->showHtmlCheckBox($c['EXPORT40']) ;
		
		return $o;
	}
	
	public function toHtmlInput(&$data, $seq=NULL, $isDisable=false)
	{
		$o = '';
		if (isset($data) && $data->HasErr())
			$o .= '<span class="field_error">*' . $data->GetErr() . '</span><br/>';
			
		$c = $this->getParsedVals($data);
		$o .= $this->showInputCheckBox($c['HIGH']) . '<br/>'
			. $this->showInputCheckBox($c['MEDIUM']) . '<br/>'
			. $this->showInputCheckBox($c['LOW']) . '<br/>'
			. $this->showInputCheckBox($c['EXPORT56']) . '<br/>'
			. $this->showInputCheckBox($c['EXPORT40']) ;
				
		return $o;
	}

}
