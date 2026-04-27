//------------------------------------------------------------------------------------------------
modded class SCR_CharacterDamageManagerComponent : SCR_DamageManagerComponent
{
	protected ACE_Medical_BrainHitZone m_ACE_Medical_BrainHitZone;
	protected ACE_Medical_VitalsComponent m_ACE_Medical_Vitals;
	protected float m_fACE_Medical_BloodFlowScale = 1;

	protected float m_fACE_Medical_LastAdrenalineHitResponseTime = 0.0;

	protected const float ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_COOLDOWN = 12.0;
	protected const float ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_MIN_DAMAGE = 0.05;
	protected const float ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_DURATION = 8.0;
	protected const float ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_DPS = -0.04;
	
	//-----------------------------------------------------------------------------------------------------------
	//! Initialize members
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!GetGame().InPlayMode())
			return;
		
		m_ACE_Medical_Vitals = ACE_Medical_VitalsComponent.Cast(owner.FindComponent(ACE_Medical_VitalsComponent));
	}

	//-----------------------------------------------------------------------------------------------------------
	//! Apply a short adrenaline response when taking damage
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (!Replication.IsServer())
			return;

		ACE_Medical_ApplyAdrenalineHitResponse(damageContext);
	}

	//-----------------------------------------------------------------------------------------------------------
	//! Uses the existing ACE epinephrine effect as a short adrenaline response.
	//! This avoids directly modifying HR/BP.
	protected void ACE_Medical_ApplyAdrenalineHitResponse(notnull BaseDamageContext damageContext)
	{
		if (damageContext.damageValue < ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_MIN_DAMAGE)
			return;

		float currentTime = GetGame().GetWorld().GetWorldTime();

		if (currentTime < m_fACE_Medical_LastAdrenalineHitResponseTime + ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_COOLDOWN)
			return;

		array<ref SCR_PersistentDamageEffect> effects = GetAllPersistentEffectsOfType(ACE_Medical_EpinephrineDamageEffect);
		if (!effects.IsEmpty())
			return;

		ACE_Medical_EpinephrineDamageEffect epiEffect = new ACE_Medical_EpinephrineDamageEffect();

		epiEffect.SetMaxDuration(ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_DURATION);
		epiEffect.SetDPS(ACE_MEDICAL_ADRENALINE_HIT_RESPONSE_DPS);

		AddDamageEffect(epiEffect);

		m_fACE_Medical_LastAdrenalineHitResponseTime = currentTime;
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Transition to cardiac arrest
	override void ACE_Medical_OnSecondChanceGranted()
	{
		// Only handle first second chance
		if (m_ACE_Medical_Vitals && !m_bACE_Medical_WasSecondChanceGranted)
			m_ACE_Medical_Vitals.SetVitalStateID(ACE_Medical_EVitalStateID.CARDIAC_ARREST);
		
		super.ACE_Medical_OnSecondChanceGranted();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Reset vitals when fully healed (e.g. by GM)
	override void FullHeal(bool ignoreHealingDOT = true)
	{
		super.FullHeal(ignoreHealingDOT);
		
		if (m_ACE_Medical_Vitals)
			m_ACE_Medical_Vitals.Reset();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Unregister from medical systems when killed
	override void ACE_Medical_OnKilled()
	{
		super.ACE_Medical_OnKilled();
		
		ACE_Medical_VitalStatesSystem system = ACE_Medical_VitalStatesSystem.GetInstance(GetOwner().GetWorld());
		if (system)
			system.Unregister(SCR_ChimeraCharacter.Cast(GetOwner()));
		
		if (m_ACE_Medical_Vitals)
		{
			m_ACE_Medical_Vitals.SetHeartRate(0);
			m_ACE_Medical_Vitals.SetCardiacOutput(0);
			m_ACE_Medical_Vitals.SetMeanArterialPressure(0);
			m_ACE_Medical_Vitals.SetPulsePressure(0);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Only allow resilience recovery in stable state
	override void ACE_Medical_UpdateResilienceRegenScale()
	{
		ACE_Medical_Circulation_Settings circulationSettings = ACE_SettingsHelperT<ACE_Medical_Circulation_Settings>.GetModSettings();
		if (!circulationSettings)
			return;
		
		if (!m_ACE_Medical_Vitals)
			return;
		
		// No recovery when vitals are not stable
		if (m_ACE_Medical_Vitals.GetVitalStateID() != ACE_Medical_EVitalStateID.STABLE)
			m_fACE_Medical_ResilienceRegenScale = 0;
		else if (m_ACE_Medical_Vitals.WasRevived())
			m_fACE_Medical_ResilienceRegenScale = circulationSettings.m_fMaxRevivalResilienceRecoveryScale;
		else
			m_fACE_Medical_ResilienceRegenScale = s_ACE_Medical_Core_Settings.m_fDefaultResilienceRegenScale;
		
		// Scale recovery scale by health of the brain hit zone
		m_fACE_Medical_ResilienceRegenScale *= m_ACE_Medical_BrainHitZone.GetHealthScaled();
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Called alpha factor in KAM
	void ACE_Medical_SetBloodFlowScale(float scale)
	{
		m_fACE_Medical_BloodFlowScale = scale;
	}
	
	//-----------------------------------------------------------------------------------------------------------
	float ACE_Medical_GetBloodFlowScale()
	{
		return m_fACE_Medical_BloodFlowScale;
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//! Called by ACE_Medical_BrainHitZone.OnInit to initialize the hit zone
	void ACE_Medical_SetBrainHitZone(HitZone hitzone)
	{
		m_ACE_Medical_BrainHitZone = ACE_Medical_BrainHitZone.Cast(hitzone);
	}

	//-----------------------------------------------------------------------------------------------------------
	//! Return the pain hit zone
	ACE_Medical_BrainHitZone ACE_Medical_GetBrainHitZone()
	{
		return m_ACE_Medical_BrainHitZone;
	}
}