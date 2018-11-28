$(document).ready(function() {
	$('.toctree-l1.current:has(.toctree-l2.current) > a').css({
		background: '#bbbbbb',
		color: 'white'
	});
});