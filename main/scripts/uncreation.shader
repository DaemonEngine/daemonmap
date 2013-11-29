textures/uncreation/dusty
{
	cull none
	entityMergable
	{
		map textures/uncreation/dust.tga
		blendFunc blend
		rgbGen		vertex
		alphaGen	vertex
	}
}
textures/uncreation/lightbridge
{
	qer_editorimage textures/uncreation/blue.tga
	qer_trans 0.40
    	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nomarks
	{
		map textures/uncreation/blue.tga
		blendFunc 	blend
		rgbGen 		identity
		alphaGen 	wave sin .2 .1 90 0.2
	}
}
textures/uncreation/fogfalloff
{
qer_editorimage textures/sfx/fog_black.tga
qer_nocarve
surfaceparm	trans
surfaceparm	nonsolid
surfaceparm	fog
surfaceparm	nolightmap
fogparms ( .03 .01 .01 ) 2048
}

//***** Shaders below are copied from the tech1soc set

textures/uncreation/grate1a_trans
{
	qer_editorimage textures/uncreation/grate1a.tga
	surfaceparm	metalsteps
	cull none
	{
		map textures/uncreation/grate1a.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}

textures/uncreation/grate1b_trans
{
	qer_editorimage textures/uncreation/grate1b.tga
	surfaceparm	metalsteps
	cull none
	{
		map textures/uncreation/grate1b.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}
textures/uncreation/lig_032-01b1-2k
{
	qer_editorimage textures/uncreation/032lig10ba.jpg
	q3map_lightimage textures/uncreation/032lig10b.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/032lig10ba.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/032lig10b.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_032-01y1-2k
{
	qer_editorimage textures/uncreation/032lig10ya.jpg
	q3map_lightimage textures/uncreation/032lig10y.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/032lig10ya.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/032lig10y.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_032-05b2-2k
{
	qer_editorimage textures/uncreation/032lig20bb.jpg
	q3map_lightimage textures/uncreation/032lig20b.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/032lig20bb.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/032lig20b.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_032-05y2-2k
{
	qer_editorimage textures/uncreation/032lig20yb.jpg
	q3map_lightimage textures/uncreation/032lig20y.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/032lig20yb.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/032lig20y.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_064-02r2-2k
{
	qer_editorimage textures/uncreation/064lig21rb.jpg
	q3map_lightimage textures/uncreation/064lig21r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/064lig21rb.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/064lig21r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_064-03r2-2k
{
	qer_editorimage textures/uncreation/064lig22rb.jpg
	q3map_lightimage textures/uncreation/064lig22r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/064lig22rb.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/064lig22r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_064z-01r1-2k
{
	qer_editorimage textures/uncreation/zdetlig01ra.jpg
	q3map_lightimage textures/uncreation/zdetlig01r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map textures/uncreation/zdetlig01ra.jpg
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/zdetlig01r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sawtooth .6 .1 0 7
	}
}
textures/uncreation/lig_128-02r1-2k
{
	qer_editorimage textures/uncreation/sqrlig03ra.jpg
	q3map_lightimage textures/uncreation/sqrlig03r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03ra.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}
textures/uncreation/lig_128-02y1-2k
{
	qer_editorimage textures/uncreation/sqrlig03ya.jpg
	q3map_lightimage textures/uncreation/sqrlig03y.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03ya.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03y.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}
textures/uncreation/lig_128-02y2-2k
{
	qer_editorimage textures/uncreation/sqrlig03yb.jpg
	q3map_lightimage textures/uncreation/sqrlig03y.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03yb.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig03y.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}
textures/uncreation/lig_128-03r1-2k
{
	qer_editorimage textures/uncreation/octlig01ra.jpg
	q3map_lightimage textures/uncreation/sqrlig02r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/octlig01ra.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig02r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}
textures/uncreation/lig_128-03r2-2k
{
	qer_editorimage textures/uncreation/octlig01rb.jpg
	q3map_lightimage textures/uncreation/sqrlig02r.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/octlig01rb.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig02r.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}
textures/uncreation/lig_128-03r3-2k
{
	qer_editorimage textures/uncreation/octlig01rc.jpg
	q3map_lightimage textures/uncreation/sqrlig02r2.blend.jpg
	q3map_surfacelight 2000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/uncreation/octlig01rc.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/uncreation/sqrlig02r2.blend.jpg
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .3 .1 0 0.5
	}
}

textures/uncreation/billsky
{
	qer_editorimage textures/uncreation/billsky_1.jpg
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap

	q3map_sun 3 2 2 70 315 65
	q3map_surfacelight 75
	skyparms - 512 -


	{
		map textures/uncreation/billsky_1.jpg
		tcMod scroll 0.05 .1
		tcMod scale 2 2
	}
	{
		map textures/uncreation/billsky_2.jpg
		blendfunc GL_ONE GL_ONE
		tcMod scroll 0.05 0.06
		tcMod scale 3 2
	}
}

textures/uncreation/clip
{
	qer_trans 0.40
	qer_editorimage common/clip.tga
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm noimpact
	surfaceparm playerclip
}
