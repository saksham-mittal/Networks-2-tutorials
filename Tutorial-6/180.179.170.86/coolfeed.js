/**
 * @version		$Id: coolfeed.js 102 2012-04-14 17:56:18Z trung3388@gmail.com $
 * @copyright	Copyright (C) 2010 - 2011 Open Source Matters, Inc. All rights reserved.
 * @author		Nguyen Quang Trung
 * @link		http://joomavatar.com
 * @license		License GNU General Public License version 2 or later
 */
 
var CoolFeed = {
	objectCF: {},
	showContent: function(feedID, pos, el)
	{
		el = $(el);
		if (el.hasClass('cf-button-enable') == false) {
			el.addClass('cf-button-enable');
		} else {
			el.removeClass('cf-button-enable');
		}
		
		var contentFeed = $$('.cf-feed-content-'+feedID);
		var myFx = new Fx.Slide(contentFeed[pos], {
		    duration: 1000,
		    resetHeight: true,
		    transition: Fx.Transitions.Pow.easeOut
		});
		myFx.toggle();
	},
	
	parentFeedObj: {},
	
	searchFeed: function(options)
	{
		CoolFeed.currentSearchID = options.feedID;
		CoolFeed.currentPrefix   = options.prefix;
		
		var parentObj 	= CoolFeed.objectCF[options.prefix + 'options-' + options.feedID]['parentObj'];
		var baseURL 	= parentObj['link'];
		var objURI 		= new URI(baseURL);
		var keyword 	= $$('.'+ options.prefix +'cf-feed-search-keyword-' + options.feedID)[0].value;
		var query 		= 'site:'+ objURI.get('host') + ' ' + keyword;
		
		google.feeds.findFeeds(query, CoolFeed.parseFeedSearchResult);
	},
	
	enableSearchFeed: function(status, options)
	{
		if (status = 1)
		{
			$$('.'+ options.prefix +'cf-feed-search-' + options.feedID).each(function(el)
			{
				el.addEvent('click', function()
				{
					CoolFeed.searchFeed(options);
				});
			});
		}
	},
	
	loadTranslateSectional: function()
	{
//		console.log(CoolFeed.currentTranslateControl);
		new google.translate.SectionalElement(
		{
	    	sectionalNodeClassName: CoolFeed.currentTranslateContent,
	    	controlNodeClassName: CoolFeed.currentTranslateControl,
	    	background: '#fff'
	  	});
	},
	
	getElement: function (parent, regex)
	{
		var els = parent.getElementsByTagName('div');
		
		for (x in els)
		{
			if (els[x].className && els[x].className.contains(regex))
			{
				return els[x];
			}
		}
		
		return null;
	},
	
	getElements: function (parent, regex)
	{
		var arrayEl = [];
		var els = parent.getElementsByTagName('div');
		
		for (x in els)
		{
			if (els[x].className && els[x].className.contains(regex))
			{
				arrayEl.push(els[x]);
			}
		}
		
		return arrayEl;
	},
	
	changeLanguage: function(me)
	{
		var parent 			 = me.parentNode;
		
		var scriptTranslate  = $('cf-script-translate');
		
		var currentControlClass = CoolFeed.getElement(parent, 'cf-feed-translate-control').className;
		var currentContentClass = CoolFeed.getElement(parent, 'cf-feed-translate-content').className;
		
		var translateControl = CoolFeed.getElement(document, currentControlClass);
		var translateContent = CoolFeed.getElement(document, currentContentClass);

		CoolFeed.currentTranslateControl = translateControl.className;
		CoolFeed.currentTranslateContent = translateContent.className;
		
		if (scriptTranslate) {scriptTranslate.src = ''; scriptTranslate.id='';}
		
		var translateControl = CoolFeed.getElements(document, 'cf-feed-translate-control');
		
		if (translateControl) 
		{
			for (x in translateControl)
			{
				translateControl[x].innerHTML = '';
			}
		}
		
		var newElement = document.createElement('script');
		newElement.id  = 'cf-script-translate';
		newElement.src = '//translate.google.com/translate_a/element.js?cb=CoolFeed.loadTranslateSectional&ug=section&hl='+ me.value;

		parent.appendChild(newElement);
	},
	
	socialShareBox: function(status, prefix)
	{
		var socialBox = $$('.cf-social-share-box')[0];
		var socialBoxDelayTime;
		
		if (!socialBox) return;
		 
		if (status == 1)
		{
			$$('.cf-social-link'+prefix).each(function(el)
			{
				var effect = new Fx.Move(socialBox, 
				{
					relativeTo: el,
					position: 'upperRight',
					offset: {x: 20, y: 0},
					onComplete: function()
					{
						$clear(socialBoxDelayTime);
						socialBoxDelayTime = socialBox.setStyles.delay(10000, socialBox ,{'display' : 'none'});
					},
					onStart: function()
					{
						socialBox.setStyles({'display' : 'block'});
					}
				});
				
				el.addEvent('mouseenter', function()
				{
					CoolFeed.linkToShare 	= encodeURI(el.href);
					CoolFeed.titleToShare 	= el.text;
					effect.start();
				});
			});
		}
		
		socialBox.addEvent('mouseenter', function(e)
		{
			$clear(socialBoxDelayTime);
		});
		
		socialBox.addEvent('mouseleave', function(e)
		{
			$clear(socialBoxDelayTime);
			socialBox.setStyles({'display' : 'none'});
		});
		
		$$('.cf-share-facebook').each(function(el)
		{
			el.addEvent('mouseenter', function()
			{
				el.href = 'http://www.facebook.com/sharer.php?u=' + CoolFeed.linkToShare + '&amp;t=' + CoolFeed.titleToShare;
			});
		});
		
		$$('.cf-share-twitter').each(function(el)
		{
			el.addEvent('mouseenter', function()
			{
				el.href = 'http://twitter.com/share?url='+ CoolFeed.linkToShare + '&text=' + CoolFeed.titleToShare;
			});
		});
	},
	
	loadFeeds: function (options) 
  	{
		var feedID		= options.feedID;
		var feedOptions = options.feedOptions;
		var containerID = options.containerID;
		var langBOX		= options.langBOX;
		var parentObj	= options.parentObj;
		var ReadMore	= options.ReadMore;
		var ClosePost	= options.ClosePost;
		feedOptions.linkRSS = feedOptions.linkRSS + ((feedOptions.linkRSS.indexOf("?") > 0) ? '&random=' : '?random=') + Math.random();
    	this.feedGoogle = new google.feeds.Feed(feedOptions.linkRSS+'?random='+Math.random());
    	this.feedGoogle.setNumEntries(feedOptions.style_number_feed); 
    	
	    this.feedGoogle.load(function(result) 
	   	{
	    	if (!result.error) 
	      	{
	        	var container 	= document.getElementById(containerID);
	        	var html 		= '';
	        	var count 		= result.feed.entries.length;
	        	var feed 		= result.feed;
	        	
	        	var parentObj = CoolFeed.objectCF[options.prefix + 'options-' + feedID]['parentObj'];
				parentObj['title'] 		= feed.title;
	        	parentObj['link'] 		= feed.link;
	        	var modal = (feedOptions.style_modal == 1) ? 'modal' : '';
	        	var modalSize = (feedOptions.style_modal_size) ? feedOptions.style_modal_size : 'x: 800, y: 600';
	        	var modalRel = (feedOptions.style_modal == 1) ? 'rel="{handler:\'iframe\', size: {'+modalSize+'} }"' : '';  
	        	
				var socialLinkClass = (feedOptions.style_show_social_share == 1) ? ' class="'+ modal +' cf-social-link'+ options.prefix + feedID +'" ' : ' class="'+ modal +'" ';
	        	var headTitleLink = '<a '+ modalRel + ' ' + socialLinkClass +' target="_blank" href="'+ feed.link +'" title="'+ feed.title +'">'+ feed.title +'</a>';
				headTitleLink	  = (feedOptions.style_show_active_link == 1) ? headTitleLink : feed.title;

				html += '<div class="cf-wrapper">';
	        	html += (feedOptions.style_show_translate == 1) ? langBOX : '';
	        	html += (feedOptions.style_show_list_title == 1) ? '	<h3 class="cf-heading">'+ headTitleLink +'</h3>' : '<br/>';
	            html += '	<ul class="cf-list-feeds">';
	
	        	for (var i = 0; i < count; i++) 
	            {
	                var entry 	= feed.entries[i];
	                var author 	= (entry.author != '' && feedOptions.style_show_author == 1) ? ' by ' + entry.author : '';
	                
	                if (entry.publishedDate && feedOptions.style_show_date == 1)
	                {
	                	 var date 			= new Date(entry.publishedDate);
	                     var datePublished 	= date.toUTCString();
	                     var dateHTML 		= '<div class="cf-feed-date">'+ date.toUTCString() + author +'</div>'; 	
	                }
	
	                var feedTitleLink = '<a '+ modalRel + ' ' + socialLinkClass +' target="_blank" href="' + entry.link + '">'+ entry.title +'</a>';
	                feedTitleLink 	  = (feedOptions.style_show_active_link == 1) ? feedTitleLink : entry.title;
	                var feedContent   = (feedOptions.style_strip_tag == 1) ? entry.content.replace(/(<([^>]+)>)/ig, '') : entry.content;
	                feedContent   	  = (feedOptions.style_limit_word_feed > 0 && feedOptions.style_strip_tag == 1) ? feedContent.substr(0, feedOptions.style_limit_word_feed) : feedContent;
	                
	                html += '<li class="cf-feed-element">';
	                html += '	<div class="cf-feed-translate-content'+ options.prefix +'">';
	                html += (feedOptions.style_show_translate == 1) ? '<div class="cf-feed-translate-control-'+ options.prefix +'"></div>' : '';
		          	html += '		<h3 class="cf-feed-heading">'+ feedTitleLink  +'</h3>';
		          	html += 		(dateHTML) ? dateHTML : '';
		          	html += (feedOptions.style_show_content == 1) ? ('<div class="cf-feed-content cf-feed-content-'+ options.prefix + feedID +'">'+ feedContent + '</div>') : '';
		          	html += '	</div>';
		          	html +=  (feedOptions.style_show_read_more == 1) ? '	<a class="cf-feed-read '+( (i == 0 && feedOptions.style_open_first_entry == 1) ? ' cf-button-enable ' : '')  +'" onclick="CoolFeed.showContent(\''+ options.prefix + feedID + '\',' + i +', this);"><span class="cf-button-read-more">' +  ReadMore + '</span><span class="cf-button-close-post">'+ ClosePost +'</span></a>' : '';
		          	html += '</li>';
				}
				
	        	html += '	</ul>';
	        	
	        	container.innerHTML = html;
	        	
	        	var contentFeed = $$('.cf-feed-content-' + options.prefix + feedID);
	    		
	    		contentFeed.each(function(el, i)
	    		{
	    			el.getElements('a').each(function(elA){
	    				elA.target = "_blank";
	    			});
	    			
	    			if (i == 0 && feedOptions.style_open_first_entry == 0) {
	    				el.slide('hide');
	    			}
	    			
		    		if (i != 0) el.slide('hide');
	    		});

	    		window.addEvent('domready', function(){
	    			CoolFeed.socialShareBox(feedOptions.style_show_social_share, options.prefix + feedID);
	    			if (feedOptions.style_modal == 1) 
	    			{
	    				SqueezeBox.initialize({});
						SqueezeBox.assign($$('a.modal'), {
							parse: 'rel'
						});	
	    			} 
	    		});
			}
		});
  	},
  	
  	parseFeedSearchResult: function(result) 
	{
  		if (!result.error) 
		{
  			var feedID 		= CoolFeed.currentSearchID;
  			var cfPrefix    = CoolFeed.currentPrefix;
  			var options 	= CoolFeed.objectCF[cfPrefix + 'options-' + feedID];
  			
			var feedOptions = options.feedOptions;
			var containerID = options.containerID;
			var langBOX		= options.langBOX;
			var parentObj	= options.parentObj;
			var ReadMore	= options.ReadMore;
			var link		= parentObj['link'];
			var title		= parentObj['title'];
			
			var container 	= document.getElementById(containerID);
	    	var count 		= result.entries.length;
	    	var html 		= '';
	    	
	    	var socialLinkClass = (feedOptions.style_show_social_share == 1) ? ' class="cf-social-link'+ cfPrefix + feedID +'" ' : '';
        	var headTitleLink 	= '<a '+ socialLinkClass +' target="_blank" href="'+ link +'" title="'+ title +'">'+ title +'</a>';
			headTitleLink	  	= (feedOptions.style_show_active_link == 1) ? headTitleLink : title;
			
			html += '<div class="cf-wrapper">';
        	html += (feedOptions.style_show_translate == 1) ? langBOX : '';
        	html += (feedOptions.style_show_list_title == 1) ? '	<h3 class="cf-heading">'+ headTitleLink +'</h3>' : '<br/>';
	    	html += '	<ul class="cf-list-feeds">';
			
	    	for (var i = 0; i < count; i++) 
	        {
	            var entry = result.entries[i];
	            var feedTitleLink = '<a '+ socialLinkClass +' target="_blank" href="' + entry.link + '">'+ entry.title +'</a>';
                feedTitleLink 	  = (feedOptions.style_show_active_link == 1) ? feedTitleLink : entry.title;
                var feedContent   = (feedOptions.style_strip_tag == 1) ? entry.contentSnippet.replace(/(<([^>]+)>)/ig, '') : entry.contentSnippet;
                feedContent   	  = (feedOptions.style_limit_word_feed > 0 && feedOptions.style_strip_tag == 1) ? feedContent.substr(0, feedOptions.style_limit_word_feed) : feedContent;
                
                html += '<li class="cf-feed-element">';
	            html += '	<div class="cf-feed-translate-content'+ options.prefix +'">';
	            html += (feedOptions.style_show_translate == 1) ? '<div class="cf-feed-translate-control-'+ options.prefix +'"></div>' : '';
	            html += '		<h3 class="cf-feed-heading">'+ feedTitleLink  +'</h3>';
	          	html += (feedOptions.style_show_content == 1) ? ('<div class="cf-feed-content cf-feed-content-'+ options.prefix + feedID +'">'+ feedContent + '</div>') : '';
	          	html += '	</div>';
	          	html += (feedOptions.style_show_read_more == 1) ? '	<a class="cf-feed-read '+( (i == 0 && feedOptions.style_open_first_entry == 0) ? ' cf-button-enable ' : '')  +'" onclick="CoolFeed.showContent(\''+ options.prefix + feedID + '\',' + i +', this);"><span class="cf-button-read-more">' +  ReadMore + '</span><span class="cf-button-close-post">'+ ClosePost +'</span></a>' : '';
	          	html += '</li>';
			}
			
	    	html += '	</ul>';
	    	
	    	container.innerHTML = html;
	
	    	var contentFeed = $$('.cf-feed-content-'+ options.prefix + feedID);
			
			contentFeed.each(function(el)
			{
				el.getElements('a').each(function(elA){
					elA.target = "_blank";
				});
				
				if (i == 0 && feedOptions.style_open_first_entry == 0) {
    				el.slide('hide');
    			}
				
				if (i != 0) el.slide('hide');
			});

			window.addEvent('domready', function(){
    			CoolFeed.socialShareBox(feedOptions.style_show_social_share, cfPrefix + feedID);
    		});
		}
	}
};