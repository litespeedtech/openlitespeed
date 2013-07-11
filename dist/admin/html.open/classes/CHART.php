<?php

class CHART {
	
	var $live = FALSE;
	var $live_delay = 1;
	var $live_url = NULL;
	
	var $y_max_value = 0;
	
	var $yaxis_label = NULL;
	
	var $data = array();
	
	function __construct($live = FALSE, $live_delay = 1, $url = NULL) {
		$this->live = (boolean) $live;
		$this->live_delay = (int) $live_delay;
		$this->live_url = trim($url);
	}
	
	function addDataSet($data, $title, $color) {
		$this->data[] = new CHART_DATA($data, $title, $color);
	}
	
	function render() {
		return $this->render_header() .  $this->render_data() . $this->render_footer();
		
	}
	
	private function render_data() {
		$buf = "";
		$buf .= "<chart_data>";
		
		$buf .="<row>\n<null/>\n";
		
		if(isset($this->data[0])) {
			for($c = count($this->data[0]->data), $i=1; $i <= $c; $i++) {
				$buf .= "<string>{$i}</string>\n";
			}
		}
		$buf .="</row>";
		
		foreach($this->data as $chart_data) {
			$buf .= "<row>\n<string>{$chart_data->title}</string>\n";
			
			foreach($chart_data->data as $value) {
				$buf .= "<number>{$value}</number>\n";	
			}
			
			$buf .= "</row>";
		}
		
		$buf .="</chart_data>";
		
		return $buf;
	
	}
	
	private function render_header() {
		$buf = "
	<chart>
	

	<axis_category size='10' color='000000' alpha='0' font='arial' bold='false' skip='0' steps='6' orientation='horizontal' />
	<axis_ticks value_ticks='true' category_ticks='true' major_thickness='1' minor_thickness='1' minor_count='1' major_color='000000' minor_color='222222' position='outside' />
	<axis_value min='0' max='{$this->y_max_value}' font='arial' bold='false' size='10' color='000000' alpha='100' steps='6' prefix='' suffix='' decimals='0' separator='' show_min='true' />

	<chart_border color='000000' top_thickness='1' bottom_thickness='1' left_thickness='1' right_thickness='1' />\n";
		
		$buf .= "<series_color>\n";

		foreach($this->data as $set) {
			$buf .= "<color>{$set->color}</color>\n";
		}
		
		$buf .= "</series_color>\n";
		
		return $buf;
	}
	
	private function render_footer() {
		
		$buf = "<chart_grid_h alpha='10' color='000000' thickness='1' type='solid' />
	<chart_grid_v alpha='10' color='000000' thickness='1' type='solid' />
	<chart_pref line_thickness='2' point_shape='none' fill_shape='false' />
	<chart_rect x='50' y='35' width='890'   positive_alpha='30' negative_color='ff0000' negative_alpha='10' />
	<chart_type>Line</chart_type>
	<chart_value position='cursor' size='12' color='ffffff' alpha='75' />

	<legend_label layout='horizontal' bold='false' size='11' alpha='90'/>
	<legend_rect x='50' y='10' width='890' height='20' margin='0' fill_alpha='8' line_color='000000' line_alpha='0' line_thickness='0'/>

<draw>
<text color='000000' alpha='60' font='arial' rotation='-90' bold='true' size='13' x='0' y='155' width='250' height='30' h_align='middle' v_align='middle'>{$this->yaxis_label}</text>
</draw>
	

	
	";
		

		if($this->live) {
			$buf .= "<live_update url='{$this->live_url}' delay='{$this->live_delay}' />\n";	
		}
		
		

		$buf .= "</chart>";
		
		return $buf;
		
	}
	
	
}

class CHART_DATA {
	
	var $data = array();
	
	//options
	var $title = NULL;
	var $color = NULL;
	
	function CHART_DATA(&$data, $title, $color) {
		$this->data = &$data;
		$this->title = &$title;
		$this->color = &$color;
	}
	
}

