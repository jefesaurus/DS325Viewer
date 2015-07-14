
#include <vector>
#include <exception>
#include <iostream>
using namespace std;

#include <DepthSense.hxx>
using namespace DepthSense;

#include <pcl/visualization/cloud_viewer.h>

static const int DEPTH_WIDTH = 320;
static const int DEPTH_HEIGHT = 240;
static const int COLOR_WIDTH = 640;
static const int COLOR_HEIGHT = 480;
const int c_PIXEL_COUNT = DEPTH_WIDTH * DEPTH_HEIGHT; // 320x240
const int c_MIN_Z = 100; // discard points closer than this
const int c_MAX_Z = 2000; // discard points farther than this

Context g_context;
DepthNode g_dnode;
ColorNode g_cnode;


bool g_bDeviceFound = false;

ProjectionHelper* g_pProjHelper = NULL;
StereoCameraParameters g_scp;

pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
pcl::visualization::CloudViewer viewer("Simple Cloud Viewer");

namespace GlobalData {
  uint16_t depth_vals[c_PIXEL_COUNT];
  uint16_t confidence_vals[c_PIXEL_COUNT];
  //uint8_t pixelsColorAcqVGA[3*c_PIXEL_COUNT];
  //uint8_t pixelsColorSyncVGA[3*c_PIXEL_COUNT];
  DepthSense::UV uv_map[c_PIXEL_COUNT];
  int colorPixelCol, colorPixelRow, colorPixelInd;
  uint32_t depth_frames = 0;
  uint32_t color_frames = 0;
}

void OnNewDepthSample(DepthNode node, DepthNode::NewSampleReceivedData data) {
  //memcpy(&GlobalData::depth_vals, data.depthMap, sizeof(data.depthMap[0]) * c_PIXEL_COUNT);
  memcpy(&GlobalData::uv_map, data.uvMap, sizeof(data.uvMap[0]) * c_PIXEL_COUNT);

  for (int i = 0; i < c_PIXEL_COUNT; i++) {
  	if (data.vertices[i].z > c_MAX_Z || data.vertices[i].z < c_MIN_Z) {
  	  cloud->points[i] = pcl::PointXYZRGB(0, 0, 0);
  	  continue;
  	}
    cloud->points[i].x = data.vertices[i].x;
    cloud->points[i].y = data.vertices[i].y;
    cloud->points[i].z = data.vertices[i].z;
  }

  GlobalData::depth_frames++;
  if (GlobalData::depth_frames <= GlobalData::color_frames) {
    viewer.showCloud(cloud);
  }
}

void OnNewColorSample(ColorNode node, ColorNode::NewSampleReceivedData data) {
  for (int depth_index = 0; depth_index < c_PIXEL_COUNT; depth_index++) {
    int color_col = ((float)GlobalData::uv_map[depth_index].u)*COLOR_WIDTH;
    int color_row = ((float)GlobalData::uv_map[depth_index].v)*COLOR_HEIGHT;
    if (color_row < 0 || color_row > COLOR_HEIGHT || color_col < 0 || color_col > COLOR_WIDTH) {
      cloud->points[depth_index].b = 0;
      cloud->points[depth_index].g = 0;
      cloud->points[depth_index].r = 0;
    } else {
      int color_index = color_row*COLOR_WIDTH + color_col;
      cloud->points[depth_index].b = data.colorMap[3*color_index + 0];
      cloud->points[depth_index].g = data.colorMap[3*color_index + 1];
      cloud->points[depth_index].r = data.colorMap[3*color_index + 2];
    }
  }

  GlobalData::color_frames++;
  if (GlobalData::color_frames <= GlobalData::depth_frames) {
    viewer.showCloud(cloud);
  }
}

void ConfigureDepthNode() {
  g_dnode.newSampleReceivedEvent().connect(&OnNewDepthSample);

  DepthNode::Configuration config = g_dnode.getConfiguration();
  config.frameFormat = FRAME_FORMAT_QVGA;
  config.framerate = 60;
  config.mode = DepthNode::CAMERA_MODE_CLOSE_MODE;
  config.saturation = true;

  g_dnode.setEnableVertices(true);
  g_dnode.setEnableUvMap(true);
  //g_dnode.setEnableDepthMap( true );
  //g_dnode.setEnableAccelerometer( true );
  //g_dnode.setEnableConfidenceMap(true); 

  try {
    g_context.requestControl(g_dnode,0);
    g_dnode.setConfiguration(config);
  } catch (ArgumentException& e) {
    printf("Argument Exception: %s\n",e.what());
  } catch (UnauthorizedAccessException& e) {
    printf("Unauthorized Access Exception: %s\n",e.what());
  } catch (IOException& e) {
    printf("IO Exception: %s\n",e.what());
  } catch (InvalidOperationException& e) {
    printf("Invalid Operation Exception: %s\n",e.what());
  } catch (ConfigurationException& e) {
    printf("Configuration Exception: %s\n",e.what());
  } catch (StreamingException& e) {
    printf("Streaming Exception: %s\n",e.what());
  } catch (TimeoutException&) {
    printf("TimeoutException\n");
  }
}


void ConfigureColorNode() {
  g_cnode.newSampleReceivedEvent().connect(&OnNewColorSample);

  ColorNode::Configuration config = g_cnode.getConfiguration();
  config.frameFormat = FRAME_FORMAT_VGA;
  config.compression = COMPRESSION_TYPE_MJPEG;
  config.powerLineFrequency = POWER_LINE_FREQUENCY_50HZ;
  config.framerate = 30;

  g_cnode.setEnableColorMap(true);

  try {
    g_context.requestControl(g_cnode,0);
    g_cnode.setConfiguration(config);
  } catch (ArgumentException& e) {
    printf("Argument Exception: %s\n",e.what());
  } catch (UnauthorizedAccessException& e) {
    printf("Unauthorized Access Exception: %s\n",e.what());
  } catch (IOException& e) {
    printf("IO Exception: %s\n",e.what());
  } catch (InvalidOperationException& e) {
    printf("Invalid Operation Exception: %s\n",e.what());
  } catch (ConfigurationException& e) {
    printf("Configuration Exception: %s\n",e.what());
  } catch (StreamingException& e) {
    printf("Streaming Exception: %s\n",e.what());
  } catch (TimeoutException&) {
    printf("TimeoutException\n");
  }
}


void ConfigureNode(Node node) {
  if ((node.is<DepthNode>())&&(!g_dnode.isSet())) {
    g_dnode = node.as<DepthNode>();
    ConfigureDepthNode();
    g_context.registerNode(node);
  } else if ((node.is<ColorNode>())&&(!g_cnode.isSet())) {
    g_cnode = node.as<ColorNode>();
    ConfigureColorNode();
    g_context.registerNode(node);
  }
}

void OnNodeConnected(Device device, Device::NodeAddedData data) {
  ConfigureNode(data.node);
}

void OnNodeDisconnected(Device device, Device::NodeRemovedData data) {
  if (data.node.is<ColorNode>() && (data.node.as<ColorNode>() == g_cnode)) {
    g_cnode.unset();
  } else if (data.node.is<DepthNode>() && (data.node.as<DepthNode>() == g_dnode)) {
    g_dnode.unset();
  }
  printf("Node disconnected\n");
}

void OnDeviceConnected(Context context, Context::DeviceAddedData data) {
  if (!g_bDeviceFound) {
    data.device.nodeAddedEvent().connect(&OnNodeConnected);
    data.device.nodeRemovedEvent().connect(&OnNodeDisconnected);
    g_bDeviceFound = true;
  }
}

void OnDeviceDisconnected(Context context, Context::DeviceRemovedData data) {
  g_bDeviceFound = false;
  printf("Device disconnected\n");
}

int main(int argc, char** argv) {
  g_context = Context::create("localhost");
  g_context.deviceAddedEvent().connect(&OnDeviceConnected);
  g_context.deviceRemovedEvent().connect(&OnDeviceDisconnected);
  cloud->points.resize(c_PIXEL_COUNT);

  // get list of devices already connected
  vector<Device> da = g_context.getDevices();

  // only use first device
  if (da.size() >= 1) {
    g_bDeviceFound = true;
    da[0].nodeAddedEvent().connect(&OnNodeConnected);
    da[0].nodeRemovedEvent().connect(&OnNodeDisconnected);
    vector<Node> na = da[0].getNodes();
    cout << "found " << (int)na.size() << " nodes\n";
    for (int n = 0; n < (int)na.size(); n++) {
      ConfigureNode(na[n]);
    }
  }

  g_context.startNodes();
  g_context.run();
  g_context.stopNodes();
  if (g_cnode.isSet()) {
    g_context.unregisterNode(g_cnode);
  }
  if (g_dnode.isSet()) {
    g_context.unregisterNode(g_dnode);
  }
  return 0;
}
