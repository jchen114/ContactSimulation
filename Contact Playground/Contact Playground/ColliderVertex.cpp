#include "stdafx.h"
#include "ColliderVertex.h"

ColliderVertex::ColliderVertex(GameObject *object, const btVector3 &offset, int vid)
{
	m_object = object;
	m_offset = offset;
	m_distanceFromCOM = m_offset.length();

	m_vertexPos = object->GetCOMPosition() + m_offset;
	m_previousPoint = m_vertexPos;
	m_vertexVel = object->GetRigidBody()->getVelocityInLocalPoint(m_offset);

	m_id = vid;
}

ColliderVertex::~ColliderVertex()
{
}

void ColliderVertex::CollisionDetectionUpdate(std::unordered_map<GameObject *, CollideeObject *> &objects) {

	if (!m_object) {
		return;
	}

	btVector3 COM = m_object->GetCOMPosition();

	m_orientation = Constants::GetInstance().DegreesToRadians(m_object->GetOrientation());

	m_newOffset = m_offset.rotate(btVector3(0, 0, 1), m_orientation);

	m_vertexPos = COM + m_newOffset;
	m_vertexVel = m_object->GetRigidBody()->getVelocityInLocalPoint(m_newOffset);
	
	//btTransform xform;
	////xform.setRotation(btQuaternion(0, 0, 0, 1));
	//xform.setOrigin(m_vertexPos);
	////xform.setBasis(btMatrix3x3(btQuaternion(btVector3(0, 0, 1), 0.0)));
	//m_ghostVertex->setWorldTransform(xform);
	//printf("m_id = %d Overlapping: %d objects \n", m_id, m_ghostVertex->getNumOverlappingObjects());

	if (m_state == IN_COLLISION) {
		ManageCollision(objects);
	}
	else {
		CheckForCollision(objects);
	}

}

void ColliderVertex::ManageCollision(std::unordered_map<GameObject *, CollideeObject *> &objects) {

	for (auto kv : objects) {
		CollideeObject* object = kv.second;

		if (object->m_object == m_contactObject) {

			m_friction = object->GetFriction();

			auto gp = object->GetGroundProperties();
			
			m_springConstant = std::get<0>(gp);
			m_dampingConstant = std::get<1>(gp);

			m_minAngle = -atan(m_friction);
			m_maxAngle = atan(m_friction);

			switch (object->m_shapeType) {
			case COLLIDEE_BOX_SHAPE: {
				//  Only implementing TOP plane for now..
				/*auto planes = object.GetPlanes();
				HandleBoxCollision(planes);*/
				HandleBoxCollision(object);
			}
				break;
			case COLLIDEE_BOX_2D_SHAPE: {
				auto planes = object->GetPlanes();
				Handle2DBoxCollision(planes);
			}
				break;
			case COLLIDEE_CIRCLE_SHAPE: {
				HandleCircleCollision(object->GetCenter(), object->GetRadius());
			}
				break;
			default:
				break;
			}

		}
	}

}

void ColliderVertex::CheckForCollision(std::unordered_map<GameObject *, CollideeObject *> &objects) {
	
	std::unordered_map<GameObject *, CollideeObject *>::iterator kv;

	//printf("num of objects = %d\n", objects.size());

	for (kv = objects.begin(); kv != objects.end(); kv ++) {
		CollideeObject* object = kv->second;
		m_friction = object->GetFriction();

		auto gp = object->GetGroundProperties();

		m_springConstant = std::get<0>(gp);
		m_dampingConstant = std::get<1>(gp);

		m_minAngle = -atan(m_friction);
		m_maxAngle = atan(m_friction);

		switch (object->m_shapeType)
		{
		case COLLIDEE_BOX_SHAPE: {
			//  Only implementing TOP plane for now..
			/*auto planes = object.GetPlanes();
			HandleBoxCollision(planes);*/
			HandleBoxCollision(object);
		}
			break;
		case COLLIDEE_BOX_2D_SHAPE: {
			auto planes = object->GetPlanes();
			Handle2DBoxCollision(planes);
		}
			break;
		case COLLIDEE_CIRCLE_SHAPE: {
			HandleCircleCollision(object->GetCenter(), object->GetRadius());
		}
			break;
		default:
			break;
		}
		if (m_state == IN_COLLISION) {
			m_contactObject = object->m_object;
			m_collideeObject = kv->second;
			break;
		}
	}
}

btVector3 ColliderVertex::GetForce() {
	return m_reactionForce;
}

CollideeObject *ColliderVertex::GetCollidingObject() {
	if (m_state == IN_COLLISION) {
		return m_collideeObject;
	}
	else {
		return nullptr;
	}
	
}

void ColliderVertex::RemoveReactionForce() {
	m_reactionForce = btVector3(0, 0, 0);
	m_collideeObject = nullptr;
	m_contactObject = nullptr;
	m_state = NO_COLLISION;
}

#pragma region HANDLERS

void ColliderVertex::HandleBoxCollision(CollideeObject* cObj) {
	// Get the location of the vertex with respect to the objects coordinates
	btVector3 relPos = cObj->m_object->GetRigidBody()->getCenterOfMassTransform().inverse()(m_vertexPos);
	btVector3 prevRelPos = cObj->m_object->GetRigidBody()->getCenterOfMassTransform().inverse()(m_previousPoint);

	auto planes = cObj->GetRelativePlanes();

	auto topPlane = planes.at(0);
	auto leftPlane = planes.at(1);
	auto rightPlane = planes.at(2);
	if (m_state == NO_COLLISION) {
		// Check where the previous point is.
		
		m_location = TOP_PLANE;

		/*if (prevRelPos.y() > std::get<0>(topPlane).y()) {
			m_location = TOP_PLANE;
		}
		if (prevRelPos.x() < std::get<0>(leftPlane).x()) {
			m_location = LEFT_PLANE;
		}
		if (prevRelPos.x() > std::get<0>(rightPlane).x()) {
			m_location = RIGHT_PLANE;
		}*/
	}
	switch (m_location) {
		case TOP_PLANE: {
			btVector3 v1 = topPlane.first;
			btVector3 v2 = topPlane.second;
			// top plane...
			if (relPos.x() > v1.x() && relPos.x() < v2.x()
				&& relPos.y() < v1.y()
				&& relPos.z() > v1.z() && relPos.z() < v2.z())
			{
				if (m_state == NO_COLLISION) {
					//printf("%d, Collision happened!\n", m_id);
					m_collisionPoint = prevRelPos;
				}
				m_state = IN_COLLISION;
				// Spring force in direction towards penetration point
				m_springForce = m_collisionPoint - relPos;
				m_springForce = m_springForce * m_springConstant;
		
				btTransform tr;
				tr.setIdentity();

				btQuaternion rotation = cObj->m_object->GetRigidBody()->getWorldTransform().getRotation();

				tr.setRotation(rotation);
				btVector3 relVel = tr.inverse()(m_vertexVel);

				m_dampingForce = relVel * m_dampingConstant;

				// Check reaction force for negative.
				m_reactionForce = m_springForce - m_dampingForce;
				m_reactionForce.y() < 0 ? m_reactionForce *= btVector3(1, 0, 0) : m_reactionForce;

				// Check to see if reaction force is inside cone of friction.
				float angle = atan(m_reactionForce.x() / m_reactionForce.y());
				if (angle < m_minAngle) { // negative angle
					m_reactionForce.setX(m_reactionForce.y() * tan(m_minAngle));
					float delta_x = (m_collisionPoint.y() - relPos.y()) * tan(m_minAngle);
					m_collisionPoint.setX(relPos.x() + delta_x);
				}
				if (angle > m_maxAngle) {
					m_reactionForce.setX(m_reactionForce.y() * tan(m_maxAngle));
					float delta_y = m_collisionPoint.y() - relPos.y();
					float x1 = delta_y * tan(m_maxAngle);
					float x2 = delta_y * tan(angle);
					float delta_x = x2 - x1;
					m_collisionPoint.setX(m_collisionPoint.x() - delta_x);
				}

				// Transform reaction force to global space.
				btVector3 t_force = tr(m_reactionForce);
				//m_reactionForce = m_reactionForce.rotate(btVector3(0, 0, 1), -degrees);

				m_object->GetRigidBody()->applyForce(t_force, m_offset);
				m_shapeInCollision = COLLIDEE_BOX_SHAPE;
			}
			else {
				m_state = NO_COLLISION;
			}
		}
		break;
		case LEFT_PLANE: {
			// check for collision with left plane
		}
		break;
		case RIGHT_PLANE: {
			// check for collision with right plane
		}
		break;
		default:
			break;
	}

	m_previousPoint = m_vertexPos;

}

void ColliderVertex::HandleBoxCollision(std::vector<std::pair<btVector3, btVector3>> planes) {
	// Check if vertex penetrates the planes.
	// Only top plane for now...
	HandleTopPlaneBoxCollision(planes.at(0));

}

void ColliderVertex::Handle2DBoxCollision(std::vector<std::pair<btVector3, btVector3>> planes) {
	std::pair < btVector3, btVector3 > top_plane = planes.front();
	btVector3 v1 = top_plane.first;
	btVector3 v2 = top_plane.second;

	// top plane... Assume horizontal
	if (m_vertexPos.x() > v1.x() 
		&& m_vertexPos.x() < v2.x()
		&& m_vertexPos.y() < v1.y())
	{
		if (m_state == NO_COLLISION) {
			//printf("%d, Collision happened!\n", m_id);
			m_state = IN_COLLISION;
			m_collisionPoint = m_previousPoint;
		}
		// Spring force in direction towards penetration point
		m_springForce = m_collisionPoint - m_vertexPos;
		//m_springForce = m_springForce.normalize();
		m_springForce = m_springForce * m_springConstant;

		m_dampingForce = m_vertexVel * m_dampingConstant;

		// Check reaction force for negative.
		m_reactionForce = m_springForce - m_dampingForce;
		m_reactionForce.y() < 0 ? m_reactionForce *= btVector3(0, 0, 0) : m_reactionForce;

		// Check to see if reaction force is inside cone of friction.
		float angle = atan(m_reactionForce.x() / m_reactionForce.y());
		if (angle < m_minAngle) { // negative angle
			m_reactionForce.setX(m_reactionForce.y() * tan(m_minAngle));
			float delta_x = (m_collisionPoint.y() - m_vertexPos.y()) * tan(m_minAngle);
			m_collisionPoint.setX(m_vertexPos.x() + delta_x);
		}
		if (angle > m_maxAngle) {
			m_reactionForce.setX(m_reactionForce.y() * tan(m_maxAngle));
			float delta_x = (m_collisionPoint.y() - m_vertexPos.y()) * tan(m_maxAngle);
			m_collisionPoint.setX(m_vertexPos.x() + delta_x);
		}
		m_object->GetRigidBody()->applyForce(m_reactionForce, m_newOffset);
		m_shapeInCollision = COLLIDEE_BOX_2D_SHAPE;
	}
	else {
		m_state = NO_COLLISION;
	}

	m_previousPoint = m_vertexPos;

}

void ColliderVertex::HandleCircleCollision(const btVector3 &center, float radius) {
	
	
	if (m_vertexPos.distance(center) <= radius) {

		if (m_state == NO_COLLISION) {
			//printf("%d, Collision happened!\n", m_id);
			m_state = IN_COLLISION;
			m_collisionPoint = m_previousPoint;
		}

		// Spring force in direction towards penetration point
		m_springForce = m_collisionPoint - m_vertexPos;
		m_springForce = m_springForce * m_springConstant;

		m_dampingForce = m_vertexVel * m_dampingConstant;

		// Check reaction force for negative.
		m_reactionForce = m_springForce - m_dampingForce;
		btVector3 vertexVector = m_vertexPos - center;
		btScalar angle = AngleBetweenVectors(vertexVector, m_reactionForce);
		// If Reaction force somehow pulls it towards the center.
		if (angle > - PI / 2 && angle < PI / 2) {
			// Do nothing
		}
		else {
			m_reactionForce = btVector3(0, 0, 0);
		}
		// Friction cone
		float angleToRotate;
		bool changeCollisionPoint = false;
		if (angle < m_minAngle) {
			angleToRotate = m_minAngle;
			changeCollisionPoint = true;
		}
		if (angle > m_maxAngle) {
			angleToRotate = m_maxAngle;
			changeCollisionPoint = true;
		}

		if (changeCollisionPoint) {
			btVector3 dir = Vector2DWithAngle(angleToRotate, vertexVector);
			dir = dir.normalize();
			dir *= m_reactionForce.norm();

			m_reactionForce = dir;

			float collisionAngle = AngleToRotateForCollision(angleToRotate, vertexVector, radius);

			btVector3 collisionVector = Vector2DWithAngle(collisionAngle, vertexVector);
			collisionVector.normalize() *= radius;

			m_collisionPoint = center + collisionVector;
			changeCollisionPoint = false;
		}
		m_object->GetRigidBody()->applyForce(m_reactionForce, m_newOffset);
		m_shapeInCollision = COLLIDEE_CIRCLE_SHAPE;
	}
	else {
		m_state = NO_COLLISION;
	}

	m_previousPoint = m_vertexPos;

}

void ColliderVertex::HandleTopPlaneBoxCollision(std::pair<btVector3, btVector3> topPlane) {
	btVector3 v1 = topPlane.first;
	btVector3 v2 = topPlane.second;
	// top plane...
	if (m_vertexPos.x() > v1.x() && m_vertexPos.x() < v2.x()
		&& m_vertexPos.y() < v1.y()
		&& m_vertexPos.z() > v1.z() && m_vertexPos.z() < v2.z())
	{
		if (m_state == NO_COLLISION) {
			//printf("%d, Collision happened!\n", m_id);
			m_collisionPoint = m_previousPoint;
		}
		m_state = IN_COLLISION;
		// Spring force in direction towards penetration point
		m_springForce = m_collisionPoint - m_vertexPos;
		m_springForce = m_springForce * m_springConstant;

		m_dampingForce = m_vertexVel * m_dampingConstant;

		// Check reaction force for negative.
		m_reactionForce = m_springForce - m_dampingForce;
		m_reactionForce.y() < 0 ? m_reactionForce *= btVector3(0, 0, 0) : m_reactionForce;

		// Check to see if reaction force is inside cone of friction.
		float angle = atan(m_reactionForce.x() / m_reactionForce.y());
		if (angle < m_minAngle) { // negative angle
			m_reactionForce.setX(m_reactionForce.y() * tan(m_minAngle));
			float delta_x = (m_collisionPoint.y() - m_vertexPos.y()) * tan(m_minAngle);
			m_collisionPoint.setX(m_vertexPos.x() + delta_x);
		}
		if (angle > m_maxAngle) {
			m_reactionForce.setX(m_reactionForce.y() * tan(m_maxAngle));
			float delta_x = (m_collisionPoint.y() - m_vertexPos.y()) * tan(m_maxAngle);
			m_collisionPoint.setX(m_vertexPos.x() + delta_x);
		}
		m_object->GetRigidBody()->applyForce(m_reactionForce, m_offset);
		m_shapeInCollision = COLLIDEE_BOX_SHAPE;
	}
	else {
		m_state = NO_COLLISION;
	}

	m_previousPoint = m_vertexPos;
}

#pragma endregion HANDLERS


#pragma region DRAWING

void ColliderVertex::DrawInfo() {
	btScalar transform[16];

	m_object->GetTransform(transform);

	glPushMatrix();

	glMultMatrixf(transform);

	glTranslatef(m_offset.x(), m_offset.y(), 0.1f);

	glColor3f(0.0f, 0.0f, 1.0f);
	DrawCircle(0.01f);

	glPopMatrix();

	//char buf[200];
	//sprintf_s(buf, "id: %d, P: (%3.3f, %3.3f, %3.3f), V:(%3.3f, %3.3f, %3.3f)", id, m_vertexPos.x(), m_vertexPos.y(), m_vertexPos.z(), m_vertexVel.x(), m_vertexVel.y(), m_vertexVel.z());
	//sprintf_s(buf, "id: %d, Col: %d, x: %3.3f, y: %3.3f, o: %f)", m_id, m_state == IN_COLLISION ? 1 : 0, m_vertexPos.x(), m_vertexPos.y(), Constants::GetInstance().RadiansToDegrees(m_orientation));

	//DisplayText(0.0f, m_id *0.2 + 0.1, btVector3(0.0f, 0.0f, 0.0f), buf);

	

	/*btTransform wt = m_ghostVertex->getWorldTransform();
	wt.getOpenGLMatrix(transform);
	glPushMatrix();
	glMultMatrixf(transform);

	glColor3f(0.0f, 0.0f, 1.0f);
	DrawCircle(0.05f);
	glPopMatrix();*/

}

void ColliderVertex::DrawForce() {

	if (m_state == IN_COLLISION) {
		
		btVector3 COM = m_object->GetCOMPosition();
		glPushMatrix();
		glTranslatef(COM.x(), COM.y(), COM.z());

		switch (m_shapeInCollision)
		{

		case COLLIDEE_BOX_SHAPE: {
			//  Only implementing TOP plane for now..
			glColor3f(0.0f, 0.0f, 1.0f);
		}
			break;
		case COLLIDEE_BOX_2D_SHAPE: {

		}
			break;
		case COLLIDEE_CIRCLE_SHAPE: {
			glColor3f(1.0f, 0.0f, 0.0f);
		}
			break;
		default:
			break;
		}

		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glVertex3f(m_newOffset.x(), m_newOffset.y(), 0.1);
		glVertex3f(m_newOffset.x() + m_reactionForce.x(), m_newOffset.y() + m_reactionForce.y(), 0.0f);
		glEnd();
		glPopMatrix();
	}

}

#pragma endregion DRAWING

float AngleToRotateForCollision(float angle, const btVector3 &vertexVector, float radius) {
	
	float numerator = sin(PI - angle) * vertexVector.norm();
	float thetaq = asin(numerator / radius);
	float angleToRotate = angle - thetaq;
	return angleToRotate;

}