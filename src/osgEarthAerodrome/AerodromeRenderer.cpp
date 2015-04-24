/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2008-2014 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "AerodromeRenderer"
#include "Common"
#include "AerodromeNode"
#include "LightBeaconNode"
#include "LightIndicatorNode"
#include "LinearFeatureNode"
#include "PavementNode"
#include "RunwayNode"
#include "RunwayThresholdNode"
#include "StartupLocationNode"
#include "StopwayNode"
#include "TaxiwayNode"
#include "TerminalNode"
#include "WindsockNode"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgEarth/ElevationQuery>
#include <osgEarth/Registry>
#include <osgEarthFeatures/Feature>
#include <osgEarthFeatures/GeometryCompiler>


using namespace osgEarth;
using namespace osgEarth::Features;
using namespace osgEarth::Aerodrome;

#define LC "[AerodromeRenderer] "


namespace
{
    class BoundingBoxMaskSource : public MaskSource
    {
    public:
        BoundingBoxMaskSource(osgEarth::Bounds bounds, Map* map)
          : MaskSource(), _bounds(bounds), _map(map)
        {
        }

        osg::Vec3dArray* createBoundary(const SpatialReference* srs, ProgressCallback* progress =0L)
        {
            osg::Vec3dArray* boundary = new osg::Vec3dArray();

            if (_map.valid())
            {
                double elevation = 0.0;

                ElevationQuery eq(_map.get());
                eq.getElevation(GeoPoint(_map->getSRS(), _bounds.center().x(), _bounds.center().y()), elevation, 0.005);
            
                boundary->push_back(osg::Vec3d(_bounds.xMin(), _bounds.yMin(), elevation));
                boundary->push_back(osg::Vec3d(_bounds.xMax(), _bounds.yMin(), elevation));
                boundary->push_back(osg::Vec3d(_bounds.xMax(), _bounds.yMax(), elevation));
                boundary->push_back(osg::Vec3d(_bounds.xMin(), _bounds.yMax(), elevation));
            }

            return boundary;
        }

    private:
        osgEarth::Bounds _bounds;
        osg::ref_ptr<Map> _map;
    };
}


AerodromeRenderer::AerodromeRenderer(Map* map)
  : _map(map), osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    //nop
}

void
AerodromeRenderer::apply(AerodromeNode& node)
{
    // reset bounds object for this new aerodrome we are about to travers
    _bounds.init();

    traverse(node);

    if (_bounds.valid())
    {
        // create ground geometry beneath the aerodrome
        double elevation = 0.0;
        ElevationQuery eq(_map.get());
        eq.getElevation(GeoPoint(_map->getSRS(), _bounds.center().x(), _bounds.center().y()), elevation, 0.005);
        
        osg::Geometry* geometry = new osg::Geometry();
        geometry->setUseVertexBufferObjects( true );
        geometry->setUseDisplayList( false );

        osg::Vec3Array* verts = new osg::Vec3Array();
        verts->reserve( 4 );

        osg::Vec3d w1;
        GeoPoint gp1(_map->getSRS(), _bounds.xMin(), _bounds.yMin(), elevation);
        gp1.toWorld(w1);

        osg::Vec3d w2;
        GeoPoint gp2(_map->getSRS(), _bounds.xMax(), _bounds.yMin(), elevation);
        gp2.toWorld(w2);

        osg::Vec3d w3;
        GeoPoint gp3(_map->getSRS(), _bounds.xMax(), _bounds.yMax(), elevation);
        gp3.toWorld(w3);

        osg::Vec3d w4;
        GeoPoint gp4(_map->getSRS(), _bounds.xMin(), _bounds.yMax(), elevation);
        gp4.toWorld(w4);

        verts->push_back(w1);
        verts->push_back(w2);
        verts->push_back(w3);
        verts->push_back(w4);

        geometry->setVertexArray( verts );

        osg::Vec4Array* colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
        geometry->setColorArray(colors);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

        //osg::Vec2Array* tcoords = new osg::Vec2Array(4);
        //(*tcoords)[3].set(0.0f,0.0f);
        //(*tcoords)[2].set(1.0f,0.0f);
        //(*tcoords)[1].set(1.0f,1.0f);
        //(*tcoords)[0].set(0.0f,1.0f);
        //geometry->setTexCoordArray(0,tcoords);

        geometry->addPrimitiveSet( new osg::DrawArrays( GL_QUADS, 0, 4 ) );

        //geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
        geometry->setName(node.icao() + "_AERODROME_TERRAIN");

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geometry);

        Registry::shaderGenerator().run(geode.get(), "osgEarth.AerodromeRenderer");

        node.addChild(geode);


        // create mask layer based on accumulated bounds
        osgEarth::MaskLayer* mask = new osgEarth::MaskLayer(osgEarth::MaskLayerOptions(), new BoundingBoxMaskSource(_bounds, _map.get()));
        node.setMaskLayer(mask);
        _map->addTerrainMaskLayer(mask);
    }
}

void
AerodromeRenderer::apply(LightBeaconNode& node)
{
    //TODO: create geometry for light beacon and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(LightIndicatorNode& node)
{
    //TODO: create geometry for light indicator and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(LinearFeatureNode& node)
{
    //TODO: create geometry for linear feature and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(PavementNode& node)
{
    //TODO: create geometry for pavement and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(RunwayNode& node)
{
    //TODO: create geometry for runway and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
    {
        geom->setName(node.icao() + "_RUNWAY_" + osgEarth::toString(feature->getFID()));
        node.addChild(geom);
    }

}

void
AerodromeRenderer::apply(RunwayThresholdNode& node)
{
    //TODO: create geometry for runway threshold and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(StartupLocationNode& node)
{
    //TODO: create geometry for startup location and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(StopwayNode& node)
{
    //TODO: create geometry for stopway and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(TaxiwayNode& node)
{
    //TODO: create geometry for taxiway and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(TerminalNode& node)
{
    //TODO: create geometry for terminal and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(WindsockNode& node)
{
    //TODO: create geometry for windsock and add to node

    osg::ref_ptr<osgEarth::Features::Feature> feature = node.getFeature();
    osg::Node* geom = randomFeatureRenderer(feature.get());
    if (geom)
        node.addChild(geom);

}

void
AerodromeRenderer::apply(osg::Group& node)
{
    // accumulate the bounds
    AerodromeFeatureNode* fnode = dynamic_cast<AerodromeFeatureNode*>(&node);
    if (fnode)
    {
        Feature* f = fnode->getFeature();
        if (f && f->getGeometry())
        {
            //TODO: make sure feature srs matches map srs (and/or do this in AerodromeFactory)
            _bounds.expandBy(f->getGeometry()->getBounds());
        }
    }

    if (dynamic_cast<AerodromeNode*>(&node))
    {
        apply(static_cast<AerodromeNode&>(node));
    }
    else
    {
        if (dynamic_cast<LightBeaconNode*>(&node))
            apply(static_cast<LightBeaconNode&>(node));
        else if (dynamic_cast<LightIndicatorNode*>(&node))
            apply(static_cast<LightIndicatorNode&>(node));
        else if (dynamic_cast<LinearFeatureNode*>(&node))
            apply(static_cast<LinearFeatureNode&>(node));
        else if (dynamic_cast<PavementNode*>(&node))
            apply(static_cast<PavementNode&>(node));
        else if (dynamic_cast<RunwayNode*>(&node))
            apply(static_cast<RunwayNode&>(node));
        else if (dynamic_cast<RunwayThresholdNode*>(&node))
            apply(static_cast<RunwayThresholdNode&>(node));
        else if (dynamic_cast<StartupLocationNode*>(&node))
            apply(static_cast<StartupLocationNode&>(node));
        else if (dynamic_cast<StopwayNode*>(&node))
            apply(static_cast<StopwayNode&>(node));
        else if (dynamic_cast<TaxiwayNode*>(&node))
            apply(static_cast<TaxiwayNode&>(node));
        else if (dynamic_cast<TerminalNode*>(&node))
            apply(static_cast<TerminalNode&>(node));
        else if (dynamic_cast<WindsockNode*>(&node))
            apply(static_cast<WindsockNode&>(node));

        traverse(node);
    }
}

osg::Node*
AerodromeRenderer::randomFeatureRenderer(osgEarth::Features::Feature* feature, float height)
{
    if (feature && _map.valid())
    {
        Session* session = new Session( _map.get() );
        GeoExtent extent(feature->getSRS(), feature->getGeometry()->getBounds());
        osg::ref_ptr<FeatureProfile> profile = new FeatureProfile( extent );
        FilterContext context(session, profile.get(), extent );

        //random color
        float r = (float)rand() / (float)RAND_MAX;
        float g = (float)rand() / (float)RAND_MAX;
        float b = (float)rand() / (float)RAND_MAX;
    
        Style style;

        if (feature->getGeometry()->getType() == osgEarth::Symbology::Geometry::TYPE_POLYGON)
        {
            style.getOrCreate<PolygonSymbol>()->fill()->color() = Color(r, g, b, 0.6);
            style.getOrCreate<LineSymbol>()->stroke()->color() = Color(r, g, b, 1.0);
        }
        else if (feature->getGeometry()->getType() == osgEarth::Symbology::Geometry::TYPE_POINTSET)
        {
            style.getOrCreate<PointSymbol>()->fill()->color() = Color(r, g, b, 1.0);
            style.getOrCreate<PointSymbol>()->size() = 5.0f;
        }
        else
        {
            style.getOrCreate<LineSymbol>()->stroke()->color() = Color(r, g, b, 1.0);
        }

        if (height > 0.0)
        {
            style.getOrCreate<ExtrusionSymbol>()->height() = height;
        }

        style.getOrCreate<AltitudeSymbol>()->clamping() = AltitudeSymbol::CLAMP_TO_TERRAIN;
        style.getOrCreate<AltitudeSymbol>()->verticalOffset() = 20.0f;
        style.getOrCreate<RenderSymbol>()->depthOffset()->minBias() = 3.6f;
        style.getOrCreate<RenderSymbol>()->depthOffset()->automatic() = false;

        GeometryCompiler compiler;
        osg::ref_ptr< Feature > clone = new Feature(*feature, osg::CopyOp::DEEP_COPY_ALL);
        return compiler.compile( clone, (clone->style().isSet() ? *clone->style() : style), context );
    }

    return 0L;
}