#include "pluginlib/class_list_macros.hpp"
#include "nav2_localization/interfaces/solver_base.hpp"
#include "nav2_localization/plugins/solver_plugins.hpp"
#include "nav2_localization/custom_particle_filter.hpp"
#include <model/systemmodel.h>
#include <model/measurementmodel.h>
#include <pdf/gaussian.h>
#include <vector>

namespace nav2_localization
{
void DummySolver2d::CreateParticleFilter(unsigned int NUM_SAMPLES, unsigned int STATE_SIZE, float PRIOR_MU_X, float PRIOR_MU_Y, float PRIOR_MU_THETA, float PRIOR_COV_X, float PRIOR_COV_Y, float PRIOR_COV_THETA)
{
	geometry_msgs::msg::TransformStamped default_pose;
	default_pose.transform.translation.x = 0.0;
	default_pose.transform.translation.y = 0.0;

	std::vector<BFL::Sample<geometry_msgs::msg::TransformStamped>> prior_samples(NUM_SAMPLES);
	for(int i=0; i<NUM_SAMPLES; i++)
		prior_samples[i].ValueSet(default_pose); //sets all particles at (0.0, 0.0)

	prior_discr_ = std::make_unique<BFL::MCPdf<geometry_msgs::msg::TransformStamped>>(NUM_SAMPLES, STATE_SIZE);
	prior_discr_->ListOfSamplesSet(prior_samples);
	
	/******************************
	 * Construction of the Filter *
	 ******************************/
	pf_ = new CustomParticleFilter(prior_discr_.get(), 0.5, NUM_SAMPLES/4.0);
}

DummySolver2d::DummySolver2d() {}

geometry_msgs::msg::TransformStamped DummySolver2d::solve(
	const geometry_msgs::msg::TransformStamped& curr_odom)
{
	// Motion update with motion sampler and current odometry
	pf_->Update(motionSampler_.get(), curr_odom);

	// Weigths calculation with matcher and measurement
	pf_->Update(matcher_.get(), *matcherPDF_->getLaserScan());

	// Get new particles (in case we want to publish them)
	// samples = pf_->getNewSamples();

	// Returns an estimated pose using all the information contained in the particle filter.
	// TODO - Add covariance to TransformStamped msg
    BFL::Pdf<geometry_msgs::msg::TransformStamped>* posterior = pf_->PostGet();
    geometry_msgs::msg::TransformStamped pose = posterior->ExpectedValueGet();
    // SymmetricMatrix pose_cov = posterior->CovarianceGet();

    return pose;
}

void DummySolver2d::configure(
	const rclcpp_lifecycle::LifecycleNode::SharedPtr& node,
	SampleMotionModelPDF::Ptr& motionSamplerPDF,
	Matcher2dPDF::Ptr& matcherPDF,
	const geometry_msgs::msg::TransformStamped& odom,
	const geometry_msgs::msg::Pose& pose)
{
	node_ = node;

	motionSamplerPDF_ = motionSamplerPDF;
	matcherPDF_ = matcherPDF;

	// Get configuration and generate PF
	int NUM_SAMPLES;
	int STATE_SIZE;
	double PRIOR_MU_X;
	double PRIOR_MU_Y;
	double PRIOR_MU_THETA;
	double PRIOR_COV_X;
	double PRIOR_COV_Y;
	double PRIOR_COV_THETA;
	node_->get_parameter("num_particles", NUM_SAMPLES);
	node_->get_parameter("num_dimensions", STATE_SIZE); // TODO - Can we fix it to 3? (or other, depending on plugin)
	node_->get_parameter("prior_mu_x", PRIOR_MU_X);
	node_->get_parameter("prior_mu_y", PRIOR_MU_Y);
	node_->get_parameter("prior_mu_theta", PRIOR_MU_THETA);
	node_->get_parameter("prior_cov_x", PRIOR_COV_X);
	node_->get_parameter("prior_cov_y", PRIOR_COV_Y);
	node_->get_parameter("prior_cov_theta", PRIOR_COV_THETA);
	CreateParticleFilter(NUM_SAMPLES, STATE_SIZE, PRIOR_MU_X, PRIOR_MU_Y, PRIOR_MU_THETA, PRIOR_COV_X, PRIOR_COV_Y, PRIOR_COV_THETA);

	motionSampler_ = std::make_shared<BFL::SystemModel<geometry_msgs::msg::TransformStamped>>(motionSamplerPDF.get());
	matcher_ = std::make_shared<BFL::MeasurementModel<sensor_msgs::msg::LaserScan, geometry_msgs::msg::TransformStamped>>(matcherPDF.get());
	prev_odom_ = odom;
	prev_pose_ = pose;
	return;
}

void DummySolver2d::activate()
{

}

void DummySolver2d::deactivate()
{

}

void DummySolver2d::cleanup()
{

}

} // nav2_localization

PLUGINLIB_EXPORT_CLASS(nav2_localization::DummySolver2d, nav2_localization::Solver)
