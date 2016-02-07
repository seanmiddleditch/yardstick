SET(HTML
	index.html
)

SET(JAVASCRIPT
	js/ysProtocol.js
	js/ysState.js
	js/ysView.js
)

SET(CSS
	css/ysView.css
)

set(YS_WEBTOOLDIR ${YS_TOOLSDIR}/web)

install(FILES ${HTML} DESTINATION ${YS_WEBTOOLDIR})
install(FILES ${JAVASCRIPT} DESTINATION ${YS_WEBTOOLDIR}/js)
install(FILES ${CSS} DESTINATION ${YS_WEBTOOLDIR}/css)